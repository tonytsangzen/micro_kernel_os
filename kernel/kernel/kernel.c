#include <dev/kdevice.h>
#include <dev/framebuffer.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/shm.h>
#include <kstring.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include <kernel/proc.h>
#include <kernel/irq.h>
#include <kernel/schedule.h>
#include <dev/timer.h>
#include <kprintf.h>
#include <vfs.h>
#include <ext2read.h>

page_dir_entry_t* _kernel_vm = NULL;

static void set_kernel_init_vm(page_dir_entry_t* vm) {
	memset(vm, 0, PAGE_DIR_SIZE);

	map_pages(vm, 0, 0, PAGE_SIZE, AP_RW_D);
	//map interrupt vector to high(virtual) mem
	map_pages(vm, INTERRUPT_VECTOR_BASE, 0, PAGE_SIZE, AP_RW_D);
	//map kernel image to high(virtual) mem
	map_pages(vm, KERNEL_BASE+PAGE_SIZE, PAGE_SIZE, V2P(_kernel_end), AP_RW_D);
	//map kernel page dir to high(virtual) mem
	map_pages(vm, (uint32_t)_kernel_vm, V2P(_kernel_vm), V2P(KMALLOC_BASE), AP_RW_D);
	//map kernel malloc mem
	map_pages(vm, KMALLOC_BASE, V2P(KMALLOC_BASE), V2P(ALLOCATABLE_MEMORY_START), AP_RW_D);
	//map MMIO to high(virtual) mem.
	map_pages(vm, MMIO_BASE, _hw_info.phy_mmio_base, _hw_info.phy_mmio_base + _hw_info.mmio_size, AP_RW_D);

  uint32_t fb_base = (uint32_t)V2P(_framebuffer_base); //framebuffer addr
  uint32_t fb_end = (uint32_t)V2P(_framebuffer_end); //framebuffer addr
  //map_pages(vm, fb_base, fb_base, base+fb_dev_get_size(), AP_RW_D);
  map_pages(vm, fb_base, fb_base, fb_end, AP_RW_D);
}

void set_kernel_vm(page_dir_entry_t* vm) {
	set_kernel_init_vm(vm);
	map_pages(vm, 
		ALLOCATABLE_MEMORY_START, 
		V2P(ALLOCATABLE_MEMORY_START),
		_hw_info.phy_mem_size,
		AP_RW_D);
}

static void init_kernel_vm(void) {
	_kernel_vm = (page_dir_entry_t*)KERNEL_PAGE_DIR;
	kalloc_init((uint32_t)_kernel_vm, KMALLOC_BASE);
	set_kernel_init_vm(_kernel_vm);
	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	__set_translation_table_base(V2P((uint32_t)_kernel_vm));
}

static void init_allocable_mem(void) {
	kalloc_init(ALLOCATABLE_PAGE_TABLES_START, ALLOCATABLE_MEMORY_START);

	map_pages(_kernel_vm,
		ALLOCATABLE_MEMORY_START,
		V2P(ALLOCATABLE_MEMORY_START),
		_hw_info.phy_mem_size,
		AP_RW_D);

	kalloc_init(ALLOCATABLE_MEMORY_START, P2V(_hw_info.phy_mem_size));
}

static void load_init(void) {
	const char* prog = "/sbin/init";
	int32_t sz;

	char* elf = sd_read_ext2(prog, &sz);
	if(elf != NULL) {
		proc_t *proc = proc_create();
		str_cpy(proc->cmd, prog);
		proc_load_elf(proc, elf, sz);
	}
	kfree(elf);
}

static void fs_init(void) {
	vfs_init();
}

void _kernel_entry_c(context_t* ctx) {
	(void)ctx;
	hw_info_init();
	init_kernel_vm();  
	printf("\n"
			"------Ewok micro-kernel-------\n"
			"kernel: %39s [ok]\n", "kernel mmu initing");

	km_init();
	printf("kernel: %39s [ok]\n", "kmalloc initing");

	init_allocable_mem(); /*init the rest allocable memory VM*/
	printf("kernel: %39s [ok]\n", "whole allocable memory initing");

	shm_init();
	printf("kernel: %39s [ok]\n", "share memory initing");

	dev_init();
	printf("kernel: %39s [ok]\n", "devices initing");

	fs_init();
	printf("kernel: %39s [ok]\n", "vfs initing");

	init_global();
	printf("kernel: %39s [ok]\n", "global env initing");

	procs_init();
	printf("kernel: %39s [ok]\n", "processes initing");

	irq_init();
	printf("kernel: %39s [ok]\n", "irq initing");

	load_init();
	printf("kernel: %39s [ok]\n", "loading first process(init)");

	timer_set_interval(0, 0x40); //0.001 sec sequence
	printf("kernel: start timer.\n");

	while(1) __asm__("MOV r0, #0; MCR p15,0,R0,c7,c0,4"); // CPU enter WFI state
}
