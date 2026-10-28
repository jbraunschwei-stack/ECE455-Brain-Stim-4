#include <assert.h>
#include <stdio.h>
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "spi_dac.h"
static bool cs;
static unsigned count, last_code;
static bool sck, mosi;
void gpio_init(unsigned pin) { assert(pin == 4); }
void gpio_put(unsigned pin, bool value) { assert(pin == 4); cs = value; }
void gpio_set_dir(unsigned pin, bool out) { assert(pin == 4 && out && cs); }
void gpio_set_function(unsigned pin, unsigned function) {
    assert(function == GPIO_FUNC_SPI);
    if (pin == 2) sck = true;
    else { assert(pin == 3); mosi = true; }
}
unsigned spi_init(void *port, unsigned baud) { assert(port == spi0 && baud > 0); return baud; }
void spi_set_format(void *port, unsigned bits, unsigned cpol, unsigned cpha, unsigned order) {
    assert(port == spi0 && bits == 8 && cpol == 1 && cpha == 1 && order == SPI_MSB_FIRST);
}
int spi_write_blocking(void *port, const uint8_t *bytes, size_t length) {
    assert(port == spi0 && !cs && length == 2 && sck && mosi);
    ++count;
    last_code = ((unsigned)bytes[0] << 8) | bytes[1];
    return (int)length;
}
int main(void) {
    spi_dac_init();
    assert(cs && count == 1 && last_code == 0x8000);
    spi_dac_write(0x8500);
    assert(cs && count == 2 && last_code == 0x8500);
    spi_dac_write(0x7B00);
    assert(cs && count == 3 && last_code == 0x7B00);
    spi_dac_zero();
    assert(cs && count == 4 && last_code == 0x8000);
    puts("PASS: SPI mode, CS framing, two-byte ordering and midscale startup");
}
