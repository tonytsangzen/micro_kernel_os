#include <dev/uart_basic.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include <kernel/proc.h>
#include <kernel/irq.h>
#include <dev/timer.h>
#include <ramfs.h>
#include <vfs.h>

page_dir_entry_t* _kernel_vm = NULL;
static uint32_t _initrd_size = 256*KB;

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
	//map initrd as read only for all proc
	map_pages(vm, (uint32_t)_initrd, V2P(_initrd), V2P(_initrd)+_initrd_size, AP_RW_R); //initrd max 256KB 
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

static ramfs_t _initfs;
static void load_initrd(void) {
	uint32_t initrd = 0x08000000;

	map_pages(_kernel_vm, initrd, initrd, initrd+_initrd_size, AP_RW_D); //initrd max 256KB 
	memcpy(_initrd, (char*)initrd, _initrd_size);
	ramfs_open(_initrd, &_initfs);
}

static void free_initrd(void) {
	ramfs_close(&_initfs);
}

static void load_init(void) {
	const char* elf = ramfs_read(&_initfs, "init", NULL);
	if(elf != NULL) {
		proc_t *proc = proc_create();
		proc_load_elf(proc, elf);
	}

	free_initrd();
}

static void fs_init(void) {
	vfs_init();
}

void _kernel_entry_c(context_t* ctx) {
	(void)ctx;
	hw_info_init();

	init_kernel_vm();  
	km_init();

	load_initrd();

	init_allocable_mem(); /*init the rest allocable memory VM*/

	irq_init();

	uart_basic_init();

	fs_init();

	procs_init();

	load_init();

	timer_set_interval(0, 1000000);
	while(1);
}
