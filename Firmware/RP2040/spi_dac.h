#ifndef SPI_DAC_H
#define SPI_DAC_H
#include <stdint.h>
/* DAC8831 SPI0 mode 3, rising-edge capture, MSB first.
 * GPIO2=SCLK, GPIO3=SDI, GPIO4=CS (ordinary output).
 * Wire H10 LDAC to board GND. H2 Howland EN remains manual. */
void spi_dac_init(void);
void spi_dac_write(uint16_t value);
void spi_dac_zero(void);
#endif
