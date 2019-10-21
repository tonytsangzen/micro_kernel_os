#include <kernel/hw_info.h>
#include <mm/mmu.h>

hw_info_t _hw_info;

void hw_info_init(void) {
	_hw_info.phy_mem_size = 256*MB;
	_hw_info.phy_mmio_base = 0x10000000;
	_hw_info.mmio_size = 4*MB;
}
