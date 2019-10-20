#include <dev/gic.h>
#include <dev/timer.h>
#include <dev/uart_basic.h>
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
	if((irqs & IRQ_UART0) != 0) {
		char c = (char)uart_basic_recv();
		printk("%c", c);
	}

	//__irq_enable();
}
