#include <stdio.h>
#include "pico/stdlib.h"
#include "spi_dac.h"

/* No automatic sweep. H2 EN remains manual. */
int main(void) {
    spi_dac_init();
    stdio_init_all();
    while (true) {
        switch (getchar_timeout_us(100000)) {
            case '0':
                spi_dac_zero();
                puts("0x8000: FILTER_OUT approximately 0 V");
                break;
            case '+':
                spi_dac_write(0x8500);
                puts("0x8500: FILTER_OUT approximately +0.100 V");
                break;
            case '-':
                spi_dac_write(0x7B00);
                puts("0x7B00: FILTER_OUT approximately -0.100 V");
                break;
            case '?':
                puts("Manual DAC8831 test: 0=zero, +=positive, -=negative.");
                puts("Keep H2 EN low until FILTER_OUT is verified. No hardware enable control.");
                break;
            default: break;
        }
    }
}
