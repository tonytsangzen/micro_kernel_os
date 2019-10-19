#include <dev/gic.h>
#include <kernel/irq.h>
#include <mm/mmu.h>

/* memory mapping for the prime interrupt controller */
#define PIC ((volatile uint32_t*)(MMIO_BASE+0x00140000))
/* interrupt controller register offsets */
#define PIC_STATUS     0
#define PIC_INTACK     3
#define PIC_ENABLE 4

/* memory mapping for the slave interrupt controller */
#define SIC ((volatile uint32_t*)(MMIO_BASE+0x00003000))
/* interrupt controller register offsets */
#define SIC_STATUS     0
#define SIC_ENABLE 2

#define PIC_INT_TIMER0 (1 << 4)
#define PIC_INT_UART0 (1 << 12)
#define PIC_INT_SIC (1 << 31)

#define SIC_INT_KEY (1 << 3)
#define SIC_INT_MOUSE (1 << 4)
#define SIC_INT_SDC (1 << 22)

void pic_set_enabled(uint32_t v) {
	PIC[PIC_ENABLE] = v;
}

uint32_t pic_get_enabled(void) {
	return PIC[PIC_ENABLE];
}

uint32_t pic_get_status(void) {
	return PIC[PIC_STATUS];
}

void sic_set_enabled(uint32_t v) {
	SIC[SIC_ENABLE] = v;
}

uint32_t sic_get_enabled(void) {
	return SIC[SIC_ENABLE];
}

uint32_t sic_get_status(void) {
	return SIC[SIC_STATUS];
}

void gic_set_irqs(uint32_t irqs) {
	uint32_t pic_en = pic_get_enabled();
	uint32_t sic_en = sic_get_enabled();
	pic_set_enabled(pic_en | PIC_INT_SIC);

  if((irqs & IRQ_TIMER0) != 0) 
		pic_set_enabled(pic_en | PIC_INT_TIMER0);
  if((irqs & IRQ_UART0) != 0) 
		pic_set_enabled(pic_en | PIC_INT_UART0);

  if((irqs & IRQ_KEY) != 0) 
		sic_set_enabled(sic_en | SIC_INT_KEY);
  if((irqs & IRQ_MOUSE) != 0) 
		sic_set_enabled(sic_en | SIC_INT_MOUSE);
  if((irqs & IRQ_SDC) != 0) 
		sic_set_enabled(sic_en | SIC_INT_SDC);
}

uint32_t gic_get_irqs(void) {
	uint32_t pic_status = PIC[PIC_STATUS];
  uint32_t sic_status = SIC[SIC_STATUS];
	uint32_t ret = 0;

  if((pic_status & PIC_INT_TIMER0) != 0) 
		ret |= IRQ_TIMER0;
  if((pic_status & PIC_INT_UART0) != 0)
		ret |= IRQ_UART0;

  if((pic_status & PIC_INT_SIC) != 0) {
    if((sic_status & SIC_INT_KEY) != 0) 
			ret |= IRQ_KEY;
    if((sic_status & SIC_INT_MOUSE) != 0) 
			ret |= IRQ_MOUSE;
    if((sic_status & SIC_INT_SDC) != 0)
			ret |= IRQ_SDC;
  }
	return ret;
}
