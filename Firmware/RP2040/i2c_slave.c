#include "i2c_slave.h"
#include "dds_control.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/i2c_slave.h"
#if DDS_ENABLE_LEAD_MONITOR
#include "hardware/adc.h"
#endif
#define I2C_PORT i2c1
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7
#define I2C_BAUDRATE 100000U
#define I2C_BASE_ADDR 0x50U
#define ADDR_PIN_0 26
#define ADDR_PIN_1 27
#define ADDR_PIN_2 28
#define RX_BUF_SIZE 16U
static uint8_t slave_addr;
static uint8_t rx_buf[RX_BUF_SIZE];
static size_t rx_count;
static bool rx_overflow;
static uint8_t command;
static uint8_t tx_buf[9];
static size_t tx_index;
static dds_control_t state;

static void prepare_status(void) {
    uint8_t fault = (g_lead_fault ? DDS_FAULT_LEAD_DISCONNECTED : 0U) |
                    (g_timing_fault ? DDS_FAULT_TIMING : 0U);
    dds_control_status(&state, fault, tx_buf);
}
static void apply_write(void) {
    if (rx_overflow || !dds_control_write(&state, rx_buf, rx_count)) return;
    g_amplitude_target = state.enabled ? calc_amp_scale(state.amplitude_uA) : 0U;
    g_phase_inc = calc_phase_inc(state.frequency_mHz);
    g_wave_type = (wave_type_t)state.waveform;
    if (command == 0xFF) g_timing_fault = false;
}
static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
        case I2C_SLAVE_RECEIVE: {
            const uint8_t byte = i2c_read_byte_raw(i2c);
            if (!rx_count) {
                command = byte;
                tx_index = 0;
                if (command == 0x06) prepare_status();
            }
            if (rx_count < RX_BUF_SIZE) rx_buf[rx_count++] = byte;
            else rx_overflow = true;
            break;
        }
        case I2C_SLAVE_REQUEST:
            i2c_write_byte_raw(i2c, command == 0x06 && tx_index < sizeof(tx_buf)
                              ? tx_buf[tx_index++] : 0xFF);
            break;
        case I2C_SLAVE_FINISH:
            if (command != 0x06) apply_write();
            rx_count = 0;
            rx_overflow = false;
            tx_index = 0;
            break;
    }
}
uint8_t i2c_slave_get_address(void) { return slave_addr; }
void app_i2c_slave_init(void) {
    dds_control_reset(&state);
    const unsigned address_pins[3] = {ADDR_PIN_0, ADDR_PIN_1, ADDR_PIN_2};
    for (unsigned i = 0; i < 3; ++i) {
        gpio_init(address_pins[i]);
        gpio_set_dir(address_pins[i], GPIO_IN);
        gpio_pull_down(address_pins[i]);
    }
    sleep_us(10);
    slave_addr = I2C_BASE_ADDR | (gpio_get(ADDR_PIN_2) << 2) |
                 (gpio_get(ADDR_PIN_1) << 1) | gpio_get(ADDR_PIN_0);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    /* External pull-ups to the appropriate 3.3 V domain are required. */
    gpio_disable_pulls(I2C_SDA_PIN);
    gpio_disable_pulls(I2C_SCL_PIN);
    i2c_init(I2C_PORT, I2C_BAUDRATE);
    i2c_slave_init(I2C_PORT, slave_addr, &i2c_slave_handler);
}
void i2c_slave_task(void) {
#if DDS_ENABLE_LEAD_MONITOR
    /* Optional existing detector input. The test PCB has no sensing circuit.
     * Never wire bipolar/high-voltage OUT directly to this ADC. */
    adc_init();
    adc_gpio_init(29);
    adc_select_input(3);
#endif
    while (true) {
#if DDS_ENABLE_LEAD_MONITOR
        g_lead_fault = g_amplitude_target > 0 && adc_read() < 75U;
#endif
        sleep_ms(20);
    }
}
