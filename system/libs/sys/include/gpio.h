#ifndef GPIO_H
#define GPIO_H

#include <types.h>

void gpio_config(int32_t gpio_num, int32_t gpio_sel);

void gpio_pull(int32_t gpio_num, int32_t pull_dir);

void gpio_write(int32_t gpio_num, int32_t value);

uint32_t gpio_read(int32_t gpio_num);

#endif
