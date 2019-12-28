#include <dev/kdevice.h>
#include <dev/actled.h>
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
#include <dev/uart.h>
#include <ext2read.h>
#include <basic_math.h>

page_dir_entry_t* _kernel_vm = NULL;
uint32_t _mmio_base = 0;

static void set_kernel_init_vm(page_dir_entry_t* vm) {
	memset(vm, 0, PAGE_DIR_SIZE);

	//map interrupt vector to high(virtual) mem
	map_pages(vm, 0, 0, PAGE_SIZE, AP_RW_D);
	map_pages(vm, INTERRUPT_VECTOR_BASE, 0, PAGE_SIZE, AP_RW_D);

	//map kernel image, page dir, kernel malloc mem
	map_pages(vm, KERNEL_BASE+PAGE_SIZE, PAGE_SIZE, V2P(ALLOCATABLE_PAGE_DIR_END), AP_RW_D);

	//map MMIO to high(virtual) mem.
	hw_info_t* hw_info = get_hw_info();
	map_pages(vm, MMIO_BASE, hw_info->phy_mmio_base, hw_info->phy_mmio_base + hw_info->mmio_size, AP_RW_D);
	arch_vm(vm);
}

void set_kernel_vm(page_dir_entry_t* vm) {
	set_kernel_init_vm(vm);
	map_pages(vm, 
		ALLOCATABLE_MEMORY_START, 
		V2P(ALLOCATABLE_MEMORY_START),
		get_hw_info()->phy_mem_size,
		AP_RW_D);
}

static void init_kernel_vm(void) {
	memset(&_ram_holes, 0, sizeof(_ram_holes));
	_kernel_vm = (page_dir_entry_t*)KERNEL_PAGE_DIR_BASE;
	//get kalloc ready just for kernel page tables.
	kalloc_init(KERNEL_PAGE_DIR_BASE+PAGE_DIR_SIZE, KERNEL_PAGE_DIR_END); 
	set_kernel_init_vm(_kernel_vm);

	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	__set_translation_table_base(V2P((uint32_t)_kernel_vm));
	_mmio_base = MMIO_BASE;
}

static void init_allocable_mem(void) {
	kalloc_init(ALLOCATABLE_PAGE_DIR_BASE, ALLOCATABLE_PAGE_DIR_END);

	map_pages(_kernel_vm,
		ALLOCATABLE_MEMORY_START,
		V2P(ALLOCATABLE_MEMORY_START),
		get_hw_info()->phy_mem_size,
		AP_RW_D);

	kalloc_init(ALLOCATABLE_MEMORY_START, P2V(get_hw_info()->phy_mem_size));
}

static int32_t load_init(void) {
	const char* prog = "/sbin/init";
	int32_t sz;

	char* elf = sd_read_ext2(prog, &sz);
	if(elf != NULL) {
		proc_t *proc = proc_create(PROC_TYPE_PROC);
		str_cpy(proc->cmd, prog);
		proc_load_elf(proc, elf, sz);
		kfree(elf);
		return 0;
	}
	return -1;
}

static void fs_init(void) {
	vfs_init();
}

void _kernel_entry_c(context_t* ctx) {
	(void)ctx;
	hw_info_init();
	init_kernel_vm();  
	uart_init();
	printf("\n"
			"------Ewok micro-kernel-------\n"
			"kernel: %39s [ok]\n", "kernel mmu initing");

	km_init();
	printf("kernel: %39s [ok] : %dMB\n", "kmalloc initing", 
		div_u32(KMALLOC_END-KMALLOC_BASE, 1*MB));

	printf("kernel: %39s ", "global env initing");
	init_global();
	printf("[ok]\n");

	printf("kernel: %39s ", "share memory initing");
	shm_init();
	printf("[ok]\n");

	printf("kernel: %39s ", "irq initing");
	irq_init();
	printf("[ok]\n");

	printf("kernel: %39s ", "devices initing");
	dev_init();

	printf("kernel: %39s ", "processes initing");
	procs_init();
	printf("[ok]\n");

	printf("kernel: %39s ", "vfs initing");
	fs_init();
	printf("[ok]\n");

	init_allocable_mem(); //init the rest allocable memory VM
	printf("kernel: %39s [ok] : %dMB\n", 
		"whole allocable memory initing", 
		div_u32(get_free_mem_size(), 1*MB));

	printf("kernel: %39s ", "loading first process(init)");
	if(load_init() != 0) {
		printf("[failed!]\n");
		while(1);
	}
	printf("[ok]\n");

	timer_set_interval(0, 0x40); //0.001 sec sequence
	printf("kernel: start timer.\n");

	while(1) __asm__("MOV r0, #0; MCR p15,0,R0,c7,c0,4"); // CPU enter WFI state
}

