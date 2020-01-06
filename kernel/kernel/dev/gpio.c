#include <dev/gpio.h>

void gpio_toggle(int32_t gpio_num) {
	if(gpio_read(gpio_num))
		gpio_clr(gpio_num);
	else 
		gpio_set(gpio_num);
}
