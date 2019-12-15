#include <dev/gic.h>
#include <dev/timer.h>
#include <dev/uart.h>
#include <dev/kdevice.h>
#include <dev/sd.h>
#include <dev/mouse.h>
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
		if(dev->io.ch.inputch == NULL || dev->io.ch.inputch(dev, 1) != 0)
			break;
		rd++;
	}
	if(rd > 0)
		proc_wakeup((uint32_t)DEV_UART0);
}

static void keyb_handler(void) {
	dev_t* dev = get_dev(DEV_KEYB);

	if(dev->io.ch.inputch != NULL)
		dev->io.ch.inputch(dev, 1);
	proc_wakeup((uint32_t)DEV_KEYB);
}

static void sd_handler(void) {
	dev_t* dev = get_dev(DEV_SD);
	sd_dev_handle(dev);
}

uint32_t _kernel_tic = 0;
static uint32_t _timer_count = 0;

void irq_handler(context_t* ctx) {
	__irq_disable();

	uint32_t irqs = gic_get_irqs();
	if((irqs & IRQ_TIMER0) != 0) {
		timer_clear_interrupt(0);

		if(_timer_count == 1000) {
			_kernel_tic++;
			_timer_count = 0;
		}
		_timer_count++;
		renew_sleep_counter();

		schedule(ctx);
		return;
	}

	if((irqs & IRQ_UART0) != 0) {
		uart_handler();
	}

	if((irqs & IRQ_KEY) != 0) {
		keyb_handler();
	}

	if((irqs & IRQ_MOUSE) != 0) {
		mouse_handler();
	}	
	
	if((irqs & IRQ_SDC) != 0) {
		sd_handler();
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
	proc_exit(ctx, _current_proc, -1);
	_current_proc = NULL;
	schedule(ctx);
}

void irq_init(void) {
	irq_arch_init();
	gic_set_irqs( IRQ_UART0 | IRQ_TIMER0 | IRQ_KEY | IRQ_MOUSE | IRQ_SDC);
	__irq_enable();
}


