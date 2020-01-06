#include <syscall.h>

void gpio_config(int32_t gpio_num, int32_t gpio_sel) {
	syscall2(SYS_GPIO_CONFIG, gpio_num, gpio_sel);
}

void gpio_pull(int32_t gpio_num, int32_t pull_dir) {
	syscall2(SYS_GPIO_PULL, gpio_num, pull_dir);
}

void gpio_write(int32_t gpio_num, int32_t value) {
	syscall2(SYS_GPIO_WRITE, gpio_num, value);
}

uint32_t gpio_read(int32_t gpio_num) {
	return syscall1(SYS_GPIO_READ, gpio_num);
}


