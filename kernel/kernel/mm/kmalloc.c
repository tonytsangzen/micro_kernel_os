#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/trunkmalloc.h>
#include <mm/mmu.h>
#include <kernel/kernel.h>
#include <kstring.h>
#include <kprintf.h>

static malloc_t _kmalloc;
static uint32_t _kmalloc_mem_tail;

/* km _exapnad expands the heap size of the given process. */
static int32_t km_expand(void *p, int32_t page_num) {
	(void)p;
	int32_t i;
	for (i = 0; i < page_num; i++) {
		char *page = kalloc4k();
		if(page == NULL) {
			printf("kernel expand failed!! free mem size: (%x), pages ask:%d\n", get_free_mem_size(), page_num);
			return -1;
		}
		memset(page, 0, PAGE_SIZE);
		map_page(_kernel_vm,
				_kmalloc_mem_tail,
				V2P(page),
				AP_RW_RW);
		_kmalloc_mem_tail += PAGE_SIZE;
	}
	return 0;
}

/* km_shrink shrinks the heap size of the given process. */
static void km_shrink(void* p, int32_t page_num) {
	(void)p;
	int32_t i;
	for (i = 0; i < page_num; i++) {
		uint32_t virtual_addr = _kmalloc_mem_tail - PAGE_SIZE;
		uint32_t physical_addr = resolve_phy_address(_kernel_vm, virtual_addr);
		//get the kernel address for kalloc4k/kfree4k
		uint32_t kernel_addr = P2V(physical_addr);
		kfree4k((void *) kernel_addr);

		unmap_page(_kernel_vm, virtual_addr);
		_kmalloc_mem_tail -= PAGE_SIZE;
		if(_kmalloc_mem_tail == ALLOCATABLE_MEMORY_START)
			break;
	}
}

static void* km_get_mem_tail(void* arg) {
	(void)arg;
	return (void*)_kmalloc_mem_tail;
}

void km_init() {
	_kmalloc_mem_tail = ALLOCATABLE_MEMORY_START;
	_kmalloc.expand = km_expand;
	_kmalloc.shrink = km_shrink;
	_kmalloc.get_mem_tail = km_get_mem_tail;
}

void *kmalloc(uint32_t size) {
	void *ret = trunk_malloc(&_kmalloc, size);
	/*if(ret == 0) {
		printk("Panic: km_alloc failed!\n");
	}
	*/
	return ret;
}

void kfree(void* p) {
	if(p == 0)
		return;
	trunk_free(&_kmalloc, p);
}
