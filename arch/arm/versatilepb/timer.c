#include <mm/mmu.h>
#include <dev/timer.h>
#include <kernel/irq.h>
#include <dev/gic.h>

/*
The ARM Versatile 926EJS board contains two ARM SB804 dual timer modules [ARM Timers 2004]. Each timer module contains two timers, which are driven by the same clock. The base addresses of the timers are:
  Timer0: 0x101E2000, Timer1: 0x101E2020
  Timer2: 0x101E3000, Timer3: 0x101E3020
*/
#define TIMER0 ((volatile uint32_t*)(MMIO_BASE+0x001e2000))
#define TIMER1 ((volatile uint32_t*)(MMIO_BASE+0x001e2020))
#define TIMER2 ((volatile uint32_t*)(MMIO_BASE+0x001e3000))
#define TIMER3 ((volatile uint32_t*)(MMIO_BASE+0x001e3020))

#define TIMER_LOAD    0x00
#define TIMER_VALUE   0x01
#define TIMER_CONTROL 0x02
#define TIMER_INTCTL  0x03
#define TIMER_BGLOAD  0x06

static volatile uint32_t* timer_addr_by_id(uint32_t id) {
	switch(id) {
		case 0:
			return TIMER0;
		case 1:
			return TIMER1;
		case 2:
			return TIMER2;
		case 3:
			return TIMER3;
	}
	return TIMER0;
}

void timer_set_interval(uint32_t id, uint32_t interval_microsecond) {
	volatile uint32_t* t = timer_addr_by_id(id);
  put8(t + TIMER_CONTROL, 0);
  put8(t + TIMER_BGLOAD, 0);
  put8(t + TIMER_LOAD, interval_microsecond);
  put8(t + TIMER_CONTROL, 0xe2);
}

void timer_clear_interrupt(uint32_t id) {
	volatile uint32_t* t = timer_addr_by_id(id);
  put32(t + TIMER_INTCTL, 0xFFFFFFFF);
}

