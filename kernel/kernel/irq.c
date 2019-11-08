#include <dev/gic.h>
#include <dev/timer.h>
#include <dev/uart_basic.h>
#include <dev/kdevicetype.h>
#include <dev/kdevice.h>
#include <kernel/irq.h>
#include <kernel/system.h>
#include <kernel/schedule.h>
#include <kernel/proc.h>
#include <string.h>
#include <kprintf.h>

static void uart_handler(void) {
	int32_t rd = 0;
	char_dev_t* dev = get_char_dev(DEV_UART0);
	while(1) {
		if(dev->ready() != 0)
			break;
		rd++;
		char c = dev->read();
		charbuf_push(&dev->buffer, c, 1);
	}
	if(rd > 0)
		proc_wakeup((uint32_t)dev);
}

void irq_handler(context_t* ctx) {
	__irq_disable();

	uint32_t irqs = gic_get_irqs();
	if((irqs & IRQ_TIMER0) != 0) {
		timer_clear_interrupt(0);
		schedule(ctx);
		return;
	}

	if((irqs & IRQ_UART0) != 0) {
		uart_handler();
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


