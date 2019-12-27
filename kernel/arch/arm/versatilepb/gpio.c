#include <dev/gpio.h>
#include <kernel/system.h>

void gpio_config(int32_t gpio_num, int32_t gpio_sel) {
	(void)gpio_num;
	(void)gpio_sel;
}

void gpio_pull(int32_t gpio_num, int32_t pull_dir) {
	(void)gpio_num;
	(void)pull_dir;
}

void gpio_set(int32_t gpio_num) {
	(void)gpio_num;
}

void gpio_clr(int32_t gpio_num) {
	(void)gpio_num;
}

void gpio_write(int32_t gpio_num, int32_t value) {
	(void)gpio_num;
	(void)value;
}

uint32_t gpio_read(int32_t gpio_num) {
	(void)gpio_num;
	return 0;
}

void gpio_toggle(int32_t gpio_num) {
	(void)gpio_num;
}
