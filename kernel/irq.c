#include <dev/gic.h>
#include <kernel/irq.h>
#include <dev/timer.h>
#include <printk.h>

void irq_handler(context_t* ctx) {
	(void)ctx;

	uint32_t irqs = gic_get_irqs();
	if((irqs & IRQ_TIMER0) != 0) {
		printk("timer0\n");
		timer_clear_interrupt(0);
	}
}
