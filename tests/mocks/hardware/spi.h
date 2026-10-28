#ifndef MOCK_SPI_H
#define MOCK_SPI_H
#include <stddef.h>
#include <stdint.h>
#define spi0 ((void *)1)
#define SPI_CPOL_1 1
#define SPI_CPHA_1 1
#define SPI_MSB_FIRST 0
unsigned spi_init(void *, unsigned);
void spi_set_format(void *, unsigned, unsigned, unsigned, unsigned);
int spi_write_blocking(void *, const uint8_t *, size_t);
#endif
