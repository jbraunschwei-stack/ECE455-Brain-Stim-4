#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "spi_dac.h"
#include "dds_state.h"
#include "i2c_slave.h"

volatile uint32_t g_phase_inc = 0;
volatile wave_type_t g_wave_type = WAVE_SINE;
volatile uint32_t g_amplitude_target = 0;
volatile bool g_lead_fault = false;
volatile bool g_timing_fault = false;

static void core1_entry(void) {
    /* Install I2C IRQs on Core 1, away from the sample loop. */
    app_i2c_slave_init();
    multicore_fifo_push_blocking(1);
    i2c_slave_task();
}
int main(void) {
    /* Keep physical H2 EN low across power-on, reset and firmware upload.
     * DAC powers up at zero code until this first midscale write. */
    spi_dac_init();
    g_phase_inc = calc_phase_inc(1000000U);
    multicore_launch_core1(core1_entry);
    multicore_fifo_pop_blocking();
    watchdog_enable(500, true);
    uint32_t phase = 0, peak_code = 0, ramp_count = 0;
    uint32_t next_sample = time_us_32() + SAMPLE_PERIOD_US;
    while (true) {
        while ((int32_t)(time_us_32() - next_sample) < 0) tight_loop_contents();
        const uint32_t now = time_us_32();
        if ((uint32_t)(now - next_sample) >= SAMPLE_PERIOD_US) {
            /* Latch a fault rather than silently run at the wrong frequency. */
            g_timing_fault = true;
            next_sample = now;
        }
        next_sample += SAMPLE_PERIOD_US;
        if (g_timing_fault || g_lead_fault) {
            peak_code = 0;
            ramp_count = 0;
        } else {
            const uint32_t target = g_amplitude_target;
            if (peak_code == target) ramp_count = 0;
            else if (++ramp_count >= AMP_RAMP_INTERVAL) {
                ramp_count = 0;
                if (peak_code < target) ++peak_code;
                else --peak_code;
            }
        }
        phase += g_phase_inc;
        const uint16_t sample = lut_table[(unsigned)g_wave_type][phase >> LUT_SHIFT];
        spi_dac_write(dds_sample_code(sample, peak_code));
        watchdog_update();
    }
}
