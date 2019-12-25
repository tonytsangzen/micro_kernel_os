#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include <dev/framebuffer.h>

static hw_info_t _hw_info;

void hw_info_init(void) {
	_hw_info.phy_mem_size = 256*MB;
	_hw_info.phy_mmio_base = 0x10000000;
	_hw_info.mmio_size = 4*MB;
}

inline hw_info_t* get_hw_info(void) {
	return &_hw_info;
}

void arch_vm(page_dir_entry_t* vm) {
  uint32_t fb_base = (uint32_t)V2P(_framebuffer_base); //framebuffer addr
  uint32_t fb_end = (uint32_t)V2P(_framebuffer_end); //framebuffer addr
  map_pages(vm, fb_base, fb_base, fb_end, AP_RW_D);
}
