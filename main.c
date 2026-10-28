#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "spi_dac.h"
#include "dds_tables.h"
#include "i2c_slave.h"
#include "hardware/adc.h"
#include "stdio.h"

/* Calibrated from 4-point measurement (1k/2k/5k/10kHz):
 * actual loop rate = avg(measured * old_constant / target) = 323819        */
#define SAMPLE_RATE 313036UL

/* Ramp interval — step amplitude by 1 count every N samples.
 * 1 count = 2000uA / 65536 = 0.03052 uA
 * At SAMPLE_RATE=323819: interval=99 → 99.82 uA/sec ≤ 100 uA/sec limit.
 * Full scale ramp (0→65536) takes ~20 seconds.                              */
#define AMP_RAMP_INTERVAL 99U

/* Watchdog timeout in milliseconds.
 * The DDS loop runs at ~323kHz so the watchdog is pet every sample.
 * 500ms gives enormous headroom — if Core 0 stalls for half a second
 * something is seriously wrong and the DAC should be shut down.             */
#define WATCHDOG_TIMEOUT_MS 500U

/* DMA buffer size — 512 samples, 3 bytes each (24-bit DAC)
 * DMA_SAMPLES generates the size for the DMA buffer, 1 byte for PD bits
 * and 2 bytes for the DAC value.                                            */
#define DMA_SAMPLES 512

/* Lead disconnection threshold */
#define LEAD_DISCONNECT_THRESHOLD 75U

/* DMA buffers will help implement DMA for SPI-DAC communication
 * One buffer is filled while the other is being transmitted.                */
static uint8_t dma_buffer_a[DMA_SAMPLES * 3];
static uint8_t dma_buffer_b[DMA_SAMPLES * 3];



/* Shared state — written by Core 1 (I2C), read by Core 0 (DDS)
 * Never write g_amplitude_target directly from the DDS loop —
 * use g_amplitude_target as the requested level; the loop ramps toward it.  */
volatile uint32_t    g_phase_inc        = 0;
volatile wave_type_t g_wave_type        = WAVE_SINE;
volatile uint32_t    g_amplitude_target = 0;      /* 0 = off, 65536 = full  */

/* Lead disconnection detection */
volatile bool g_lead_fault = false;  /* true if lead is disconnected */
volatile uint16_t g_lead_adc_raw = 0;  /* raw ADC reading for lead disconnection detection */

static void fill_buffer(uint8_t *buffer, uint32_t *phase_acc, uint32_t amp_scale){

    const uint16_t *lut = lut_table[(uint8_t)g_wave_type];
    
    for(uint32_t i = 0; i < DMA_SAMPLES; i++) {
        *phase_acc += g_phase_inc;

        uint32_t idx = *phase_acc >> LUT_SHIFT;

        uint16_t sample = (uint16_t)(((uint32_t)lut[idx] * amp_scale) >> 16);

        buffer[i * 3 + 0] = 0x00;
        buffer[i * 3 + 1] = sample >> 8;
        buffer[i * 3 + 2] = sample & 0xFF;
    }

}

static uint32_t calc_phase_inc(uint32_t freq_hz) {
    return (uint32_t)(freq_hz * 4294967296ULL / SAMPLE_RATE);
}

/* Core 1 — handles I2C slave forever */
void core1_entry(void) {
    i2c_slave_task();
}

int main(void) {
    stdio_init_all();

      gpio_init(2);
    gpio_set_dir(2, GPIO_OUT);

    /* ── Watchdog-reset safety shutdown ─────────────────────────────────────
     * If we're restarting after a watchdog timeout, the DDS loop stalled
     * while the DAC may have been outputting. Power it down immediately
     * before doing anything else — before even initialising SPI fully.
     * spi_dac_init() is called right after to bring it back to a clean state.*/
    if (watchdog_caused_reboot()) {
        spi_dac_init();
        spi_dac_powerdown();   /* tristate output — safe, high-impedance */
    }

    sleep_ms(10);

    spi_dac_init();
    app_i2c_slave_init();

    /*Initialize ADC for lead disconnection detection*/
    adc_init();
    adc_gpio_init(29);
    adc_select_input(3);

    /* Default: 1kHz sine, output off (target=0) until ESP32 enables */
    g_wave_type        = WAVE_SINE;
    g_phase_inc        = calc_phase_inc(1000);
    g_amplitude_target = 0;

    multicore_launch_core1(core1_entry);

    /* Enable watchdog — must be pet at least once every WATCHDOG_TIMEOUT_MS.
     * pause_on_debug=true so the watchdog doesn't fire during a debug halt.  */
    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);

    /* Core 0: free-running DDS loop with amplitude ramp.
     * s_amp_scale tracks the currently applied Q16 scale and steps toward
     * g_amplitude_target by 1 count every AMP_RAMP_INTERVAL samples,
     * enforcing the ≤ 100 uA/sec ramp limit.
     * Phase accumulator always advances — no hard output cutoff ever.
     * watchdog_update() is called every sample — at 323kHz this pets the
     * watchdog ~323000 times per second, well within the 500ms timeout.      */
    uint32_t phase_acc  = 0;
    uint32_t amp_scale  = 0;   /* currently applied scale */
    uint32_t ramp_count = 0;   /* interval counter */

    uint32_t lead_check_counter = 0;  /* counter for lead disconnection detection */
    
    while (true) {
        /* ── Amplitude ramp step ────────────────────────────────────────── */
        uint32_t target = g_amplitude_target;
        if (amp_scale != target) {
            if (++ramp_count >= AMP_RAMP_INTERVAL) {
                ramp_count = 0;
                if (amp_scale < target) {
                    amp_scale++;
                } else {
                    amp_scale--;
                }
            }
        } else {
            ramp_count = 0;
        }        

        /* ── DDS output — always running ────────────────────────────────── */
        phase_acc += g_phase_inc;
        uint32_t idx = phase_acc >> LUT_SHIFT;
        const uint16_t *lut = lut_table[(uint8_t)g_wave_type];
        uint16_t sample = (uint16_t)(((uint32_t)lut[idx] * amp_scale) >> 16);
        spi_dac_write(sample);

        /* ── Lead disconnection detection ───────────────────────────────── */
        if(lead_check_counter >= 100000) {
            lead_check_counter = 0;
            g_lead_adc_raw = adc_read();

            /*Only check for lead disconnection when amplitude is being output */
            if(g_amplitude_target > 0 && g_lead_adc_raw < LEAD_DISCONNECT_THRESHOLD) {
                g_lead_fault = true;  /* lead is disconnected */
            } else {
                g_lead_fault = false;  /* lead is connected */
            }

            // for debugging and reading ADC values
            // float voltage = (g_lead_adc_raw * 3.3f) / 4095.0f;  /* Convert ADC reading to voltage */
            // printf("ADC=%u Voltage=%.3f V Fault = %d\n", g_lead_adc_raw, voltage, g_lead_fault);
            

        } else {
            lead_check_counter++;
        }

        /* ── Pet the watchdog ───────────────────────────────────────────── */
        watchdog_update();
    } 
}