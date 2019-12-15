#include <dev/gic.h>
#include <kernel/irq.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <arch.h>

/* memory mapping for the prime interrupt controller */
#define PIC (MMIO_BASE + 0xB200)

#define PIC_INT_TIMER0 (1 << 0)
#define PIC_INT_UART0 ((1 << 29) - 64)

static pic_regs_t* _pic;

void irq_arch_init(void) {
	_pic = (pic_regs_t*)(PIC);
}

void gic_set_irqs(uint32_t irqs) {
  if((irqs & IRQ_TIMER0) != 0) 
		_pic->enable |= PIC_INT_TIMER0;
  if((irqs & IRQ_UART0) != 0) 
		_pic->enable |= PIC_INT_UART0;
}

uint32_t gic_get_irqs(void) {
	uint32_t ret = 0;
  if((_pic->status & PIC_INT_TIMER0) != 0) 
		ret |= IRQ_TIMER0;
  if((_pic->status & PIC_INT_UART0) != 0)
		ret |= IRQ_UART0;

	return ret;
}
