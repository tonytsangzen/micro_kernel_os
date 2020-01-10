#ifndef GPIO_H
#define GPIO_H

#include <types.h>

int32_t gpio_config(int32_t fd, int32_t gpio_num, int32_t gpio_sel);

int32_t gpio_pull(int32_t fd, int32_t gpio_num, int32_t pull_dir);

int32_t gpio_write(int32_t fd, int32_t gpio_num, int32_t value);

uint32_t gpio_read(int32_t fd, int32_t gpio_num);

#endif
