#ifndef TIMER_H
#define TIMER_H

extern void timer_set_interval(uint32_t id, uint32_t interval_microsecond);

extern void timer_clear_interrupt(uint32_t id);

extern void timer_init(void);

#endif
