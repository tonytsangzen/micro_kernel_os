#include <kernel/arch_info.h>
#include <mm/mmu.h>

arch_info_t _arch_info;

void arch_info_init(void) {
	_arch_info.phy_mem_size = 256*MB;
	_arch_info.phy_mmio_base = 0x10000000;
	_arch_info.mmio_size = 4*MB;
}
