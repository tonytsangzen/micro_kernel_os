#include <dev/gic.h>
#include <kernel/irq.h>
#include <dev/uart.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <arch.h>
#include "timer_arch.h"

/* memory mapping for the prime interrupt controller */
#define PIC (MMIO_BASE + 0xB200)

#define PIC_INT_UART0 (25+64)

static pic_regs_t* _pic;

#define IRQ_IS_BASIC(x) ((x) >= 64 )
#define IRQ_IS_GPU2(x) ((x) >= 32 && (x) < 64 )
#define IRQ_IS_GPU1(x) ((x) < 32 )

/*
static void enable_irq(uint32_t id) {
	uint32_t irq_pos;
	if(IRQ_IS_BASIC(id)) {
		irq_pos           = id - 64;
		_pic->irq_basic_enable |= (1 << irq_pos);
	}
	else if (IRQ_IS_GPU2(id)) {
		irq_pos           = id - 32;
		_pic->irq_gpu_enable2 |= (1 << irq_pos);
	}
	else if (IRQ_IS_GPU1(id)) {
		irq_pos           = id;
		_pic->irq_gpu_enable1 |= (1 << irq_pos);
	}
}
*/

#define CORE0_TIMER__irqCNTL 0x40000040

static void routing_core0cntv_to_core0irq(void) {
  uint32_t offset = CORE0_TIMER__irqCNTL - _hw_info.phy_mmio_base;
  uint32_t vbase = MMIO_BASE+offset;
  put32(vbase, 0x08);
}

#define CORE0__irq_SOURCE 0x40000060
static uint32_t read_core0timer_pending(void) {
  uint32_t tmp;
  uint32_t offset = CORE0__irq_SOURCE -  _hw_info.phy_mmio_base;
  uint32_t vbase = MMIO_BASE+offset;
  tmp = get32(vbase);
  return tmp;
}

#define CORE0_ROUTING 0x40000000
void irq_arch_init(void) {
	_pic = (pic_regs_t*)(PIC);
	uint32_t offset = CORE0_ROUTING - _hw_info.phy_mmio_base;
	uint32_t vbase = MMIO_BASE + offset;
	uint32_t pbase = _hw_info.phy_mmio_base + offset;
	map_pages(_kernel_vm, vbase, pbase, pbase+16*KB, AP_RW_D);
}

void gic_set_irqs(uint32_t irqs) {
  if((irqs & IRQ_TIMER0) != 0) 
		routing_core0cntv_to_core0irq();
  //if((irqs & IRQ_UART0) != 0)  
	//	enable_irq(PIC_INT_UART0);
}

uint32_t gic_get_irqs(void) {
	uint32_t ret = 0;
	if (read_core0timer_pending() & 0x08 ) {
		ret |= IRQ_TIMER0;
		write_cntv_tval(_timer_frq); 
	}
  //if((_pic->irq_basic_pending & PIC_INT_UART0) != 0)

	if(uart_ready_to_recv() == 0) {
		ret |= IRQ_UART0;
	}
	return ret;
}
