#include <dev/gic.h>
#include <dev/timer.h>
#include <printk.h>
#include <kernel/irq.h>
#include <kernel/system.h>
#include <kernel/schedule.h>

void irq_handler(context_t* ctx) {
	(void)ctx;
	__irq_disable();

	uint32_t irqs = gic_get_irqs();
	if((irqs & IRQ_TIMER0) != 0) {
		timer_clear_interrupt(0);
		schedule(ctx);
	}

	__irq_enable();
}
