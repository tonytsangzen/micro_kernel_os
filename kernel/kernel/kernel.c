#include <dev/uart_basic.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <printk.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <kernel/arch_info.h>
#include <kernel/proc.h>
#include <kernel/irq.h>
#include <dev/timer.h>

static page_dir_entry_t* _kernel_vm;

static uint32_t _phy_mem_size = 0;

void set_kernel_vm(page_dir_entry_t* vm) {
	memset(vm, 0, PAGE_DIR_SIZE);

	map_pages(vm, 0, 0, PAGE_SIZE, AP_RW_D);
	//map interrupt vector to high(virtual) mem
	map_pages(vm, INTERRUPT_VECTOR_BASE, 0, PAGE_SIZE, AP_RW_D);
	//map kernel image to high(virtual) mem
	map_pages(vm, KERNEL_BASE+PAGE_SIZE, PAGE_SIZE, V2P(_kernel_end), AP_RW_D);
	//map kernel page dir to high(virtual) mem
	map_pages(vm, (uint32_t)_kernel_vm, V2P(_kernel_vm), V2P(_kernel_vm)+PAGE_DIR_SIZE, AP_RW_D);
	//map MMIO to high(virtual) mem.
	map_pages(vm, MMIO_BASE, _arch_info.phy_mmio_base, _arch_info.phy_mmio_base + _arch_info.mmio_size, AP_RW_D);
	//map kernel memory trunk to high(virtual) mem.
	map_pages(vm, KMALLOC_BASE, V2P(KMALLOC_BASE), V2P(KMALLOC_BASE+KMALLOC_SIZE), AP_RW_D);
	
	if(_phy_mem_size == 0) {
		/*map some allocable memory for the pagetable alloc for rest momory mapping
		(coz we don't know the whole phymem size yet.*/
		map_pages(vm, 
			ALLOCATABLE_MEMORY_START,
			V2P(ALLOCATABLE_MEMORY_START),
			V2P(ALLOCATABLE_MEMORY_START) + INIT_RESERV_MEMORY_SIZE,
			AP_RW_D);
	}
	else {
		map_pages(vm, 
			ALLOCATABLE_MEMORY_START, 
			V2P(ALLOCATABLE_MEMORY_START),
			_phy_mem_size,
			AP_RW_D);
	}
}

void __set_translation_table_base(uint32_t v);

static void init_kernel_vm(void) {
	/*
	build free mems list only for kernel init.
	We can only use init memory part
	(from ALLOCATABLE_MEMORY_START to 'KERNEL_BASE + INIT_MEMORY_SIZE'),
	cause the boot program only mapped part of mem by _startup_page_dir(startup.c).
	Notice: This part of physical mem (0 to INIT_MEMORY_SIZE) works for init kernel page mapping
	*/
	kalloc_init(ALLOCATABLE_MEMORY_START,
			ALLOCATABLE_MEMORY_START + INIT_RESERV_MEMORY_SIZE);


	//align up to PAGE_DIR_SIZE (like 16KB in this case). 16KB memory after kernel be used for kernel page dir table 
	_kernel_vm = (page_dir_entry_t*)ALIGN_UP((uint32_t)_kernel_end, PAGE_DIR_SIZE);

	set_kernel_vm(_kernel_vm);
	
	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	__set_translation_table_base(V2P((uint32_t)_kernel_vm));

	/*
	init kernel trunk memory malloc.
	*/
	km_init();
}

static void init_allocable_mem(void) {
	_phy_mem_size = _arch_info.phy_mem_size;

	map_pages(_kernel_vm,
		ALLOCATABLE_MEMORY_START, 
		V2P(ALLOCATABLE_MEMORY_START),
		_phy_mem_size,
		AP_RW_D);

	kalloc_init(ALLOCATABLE_MEMORY_START + INIT_RESERV_MEMORY_SIZE, P2V(_phy_mem_size));
}

extern char _initrd[];
static void load_initrd(void) {
	uint32_t initrd = 0x08000000;
	map_pages(_kernel_vm, initrd, initrd, initrd+0x40000, AP_RW_D);
	memcpy(_initrd, (char*)initrd, 1*MB);
}

static void load_init(void) {
	proc_t *proc = proc_create();
	proc_load_elf(proc, _initrd);
}

void _kernel_entry_c(void) {
	arch_info_init();
	init_kernel_vm();  /* Done mapping all mem */
	load_initrd();
	init_allocable_mem(); /*init the rest allocable memory VM*/

	irq_init();

	uart_basic_init();

	procs_init();
	load_init();

	timer_set_interval(0, 1000000);
	while(1);
}
