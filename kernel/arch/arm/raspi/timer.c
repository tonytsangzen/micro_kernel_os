#include <mm/mmu.h>
#include <dev/timer.h>
#include <kernel/irq.h>
#include <basic_math.h>

#define ARM_TIMER_LOD (_mmio_base+0x0B400)
#define ARM_TIMER_VAL (_mmio_base+0x0B404)
#define ARM_TIMER_CTL (_mmio_base+0x0B408)
#define ARM_TIMER_CLI (_mmio_base+0x0B40C)
#define ARM_TIMER_RIS (_mmio_base+0x0B410)
#define ARM_TIMER_MIS (_mmio_base+0x0B414)
#define ARM_TIMER_RLD (_mmio_base+0x0B418)
#define ARM_TIMER_DIV (_mmio_base+0x0B41C)
#define ARM_TIMER_CNT (_mmio_base+0x0B420)

void timer_set_interval(uint32_t id, uint32_t interval_microsecond) {
	(void)id;
  put32(ARM_TIMER_CTL,0x003E0000);
	put32(ARM_TIMER_LOD,interval_microsecond*10-1);
	put32(ARM_TIMER_RLD,interval_microsecond*10-1);
  put32(ARM_TIMER_CLI,0);
  put32(ARM_TIMER_CTL,0x003E00A2);
  put32(ARM_TIMER_CLI,0);
}

void timer_clear_interrupt(uint32_t id) {
	(void)id;
	put32(ARM_TIMER_CLI,0);
}

