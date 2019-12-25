#include <dev/actled.h>
#include "gpio.h"

void act_led(bool on) {
	uint32_t ra;
	ra = get32(GPFSEL4);
	ra &= ~(7<<21);
	ra |= 1<<21;
	put32(GPFSEL4, ra);

	if(on) 	
		put32(GPCLR1, 1<<(47-32));
	else
		put32(GPSET1, 1<<(47-32));
}
