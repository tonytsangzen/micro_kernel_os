#include <dev/gic.h>
#include <dev/timer.h>
#include <dev/uart_basic.h>
#include <dev/devicetype.h>
#include <dev/device.h>
#include <kernel/irq.h>
#include <kernel/system.h>
#include <kernel/schedule.h>
#include <kernel/proc.h>
#include <string.h>
#include <kprintf.h>

void irq_handler(context_t* ctx) {
	__irq_disable();

	uint32_t irqs = gic_get_irqs();
	if((irqs & IRQ_TIMER0) != 0) {
		timer_clear_interrupt(0);
		schedule(ctx);
		return;
	}

	if((irqs & IRQ_UART0) != 0) {
		charbuf_t* buf = dev_getbuf(DEV_UART0);
		while(1) {
			if(uart_basic_ready_to_recv() == 0)
				break;
			char c = (char)uart_basic_recv();
			charbuf_push(buf, c, 1);
		}
	}
}

void prefetch_abort_handler(context_t* ctx) {
	(void)ctx;
	printf("pid: %d, prefetch abort!!\n", _current_proc->pid);
	while(1);
}

void data_abort_handler(context_t* ctx) {
	(void)ctx;
	printf("pid: %d, data abort!!\n", _current_proc->pid);
	while(1);
}

void irq_init(void) {
	gic_set_irqs( IRQ_UART0 | IRQ_TIMER0 | IRQ_KEY);
	__irq_enable();
}


