#include <dev/gic.h>
#include <dev/timer.h>
#include <dev/uart_basic.h>
#include <kernel/irq.h>
#include <kernel/system.h>
#include <kernel/schedule.h>

void irq_handler(context_t* ctx) {
	(void)ctx;
	//__irq_disable();

	uint32_t irqs = gic_get_irqs();
	if((irqs & IRQ_TIMER0) != 0) {
		timer_clear_interrupt(0);
		schedule(ctx);
		return;
	}

	if((irqs & IRQ_UART0) != 0) {
		char c = (char)uart_basic_recv();
		uart_basic_putch(c);
	}
}

void irq_init(void) {
	gic_set_irqs( IRQ_UART0 | IRQ_TIMER0 | IRQ_KEY);
	__irq_enable();
}
