#ifndef ACTLED_H
#define ACTLED_H

#include <types.h>
#include <kernel/system.h>

void act_led(bool on);

static inline void flush_led(void) {
	act_led(true);
	_delay(0x100000);
	act_led(false);
	_delay(0x100000);
}

#endif
