#ifndef MOCK_STDLIB_H
#define MOCK_STDLIB_H
#include <stdbool.h>
#define GPIO_OUT 1
#define GPIO_FUNC_SPI 1
void gpio_init(unsigned);
void gpio_put(unsigned, bool);
void gpio_set_dir(unsigned, bool);
void gpio_set_function(unsigned, unsigned);
#endif
