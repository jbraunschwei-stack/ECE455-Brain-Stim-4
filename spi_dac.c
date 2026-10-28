#include "spi_dac.h"
#include "dds_state.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#define SCK_PIN 2
#define MOSI_PIN 3
#define CS_PIN 4
#ifndef DAC_SPI_BAUD_HZ
#define DAC_SPI_BAUD_HZ 10000000U
#endif

void spi_dac_write(uint16_t value) {
    const uint8_t bytes[2] = {(uint8_t)(value >> 8), (uint8_t)value};
    gpio_put(CS_PIN, 0);
    spi_write_blocking(spi0, bytes, 2);
    /* Blocking SDK call waits for SPI busy to clear before returning. */
    gpio_put(CS_PIN, 1);
}
void spi_dac_zero(void) { spi_dac_write(DAC_ZERO_CODE); }
void spi_dac_init(void) {
    gpio_init(CS_PIN);
    gpio_put(CS_PIN, 1);
    gpio_set_dir(CS_PIN, GPIO_OUT);
    spi_init(spi0, DAC_SPI_BAUD_HZ);
    spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    gpio_set_function(SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);
    spi_dac_zero();
}
