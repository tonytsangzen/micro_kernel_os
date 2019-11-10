#include <dev/gic.h>
#include <dev/timer.h>
#include <dev/uart.h>
#include <dev/kdevice.h>
#include <kernel/irq.h>
#include <kernel/system.h>
#include <kernel/schedule.h>
#include <kernel/proc.h>
#include <string.h>
#include <kprintf.h>

static void uart_handler(void) {
	int32_t rd = 0;
	dev_t* dev = get_dev(DEV_UART0);
	while(1) {
		if(dev->inputch == NULL || dev_ready(dev) != 0)
			break;
		rd++;
		dev->inputch(dev, 1);
	}
	if(rd > 0)
		proc_wakeup((uint32_t)dev);
}

static void keyb_handler(void) {
	dev_t* dev = get_dev(DEV_KEYB);

	if(dev->inputch != NULL)
		dev->inputch(dev, 1);
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

	if((irqs & IRQ_KEY) != 0) {
		keyb_handler();
	}
}

void prefetch_abort_handler(context_t* ctx) {
	(void)ctx;
	printf("pid: %d(%s), prefetch abort!!\n", _current_proc->pid, CS(_current_proc->cmd));
	while(1);
}

void data_abort_handler(context_t* ctx) {
	(void)ctx;
	printf("pid: %d(%s), data abort!!\n", _current_proc->pid, CS(_current_proc->cmd));
	while(1);
}

void irq_init(void) {
	gic_set_irqs( IRQ_UART0 | IRQ_TIMER0 | IRQ_KEY);
	__irq_enable();
}


