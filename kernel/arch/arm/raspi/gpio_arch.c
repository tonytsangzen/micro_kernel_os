#include "gpio_arch.h"
#include <dev/gpio.h>
#include <kernel/system.h>

void gpio_config(int32_t gpio_num, int32_t gpio_sel) {
	uint32_t raddr = (uint32_t)GPIO_FSEL0 + ((gpio_num/10)<<2);
	uint32_t shift = (gpio_num%10) * GPIO_SEL_BITS;
	uint32_t value = gpio_sel << shift;
	uint32_t mask = GPIO_SEL << shift;
	uint32_t data = get32(raddr);
	data &= ~mask;
	data |= value;
	put32(raddr, data);
}

void gpio_pull(int32_t gpio_num, int32_t pull_dir) {
	uint32_t shift = (gpio_num % 32);
	uint32_t index = (gpio_num/32) + 1;
	*GPIO_PUD = pull_dir & GPIO_PULL_MASK;
	_delay(150);
	put32((uint32_t)GPIO_PUD+(index<<2), 1<<shift); /* enable ppud clock */
	_delay(150);
	*GPIO_PUD = GPIO_PULL_NONE;
	put32((uint32_t)GPIO_PUD+(index<<2), 0); /* disable ppud clock */
}

inline void gpio_set(int32_t gpio_num) {
	put32((uint32_t)GPIO_SET0 + ((gpio_num/32)<<2),1<<(gpio_num%32));
}

inline void gpio_clr(int32_t gpio_num) {
	put32((uint32_t)GPIO_CLR0+((gpio_num/32)<<2),1<<(gpio_num%32));
}

inline void gpio_write(int32_t gpio_num, int32_t value) {
	if(value)
		gpio_set(gpio_num);
	else 
		gpio_clr(gpio_num);
}

inline uint32_t gpio_read(int32_t gpio_num) {
	return get32((uint32_t)GPIO_LEV0 + ((gpio_num/32)<<2))&(1<<(gpio_num%32));
}

