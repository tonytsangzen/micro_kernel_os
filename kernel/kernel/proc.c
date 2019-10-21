#include <kernel/system.h>
#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <kernel/schedule.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <printk.h>
#include <elf.h>

static proc_t _proc_table[PROC_MAX];
__attribute__((__aligned__(PAGE_DIR_SIZE))) 
static page_dir_entry_t _proc_vm[PROC_MAX][PAGE_DIR_NUM];
proc_t* _current_proc = NULL;

/* proc_init initializes the process sub-system. */
void procs_init(void) {
	int32_t i;
	for (i = 0; i < PROC_MAX; i++)
		_proc_table[i].state = UNUSED;
	_current_proc = NULL;
}

proc_t* proc_get_next_ready(void) {
	int32_t i = 0;
	int32_t end = 0;
	if(_current_proc != NULL) {
		end = _current_proc->pid;
		i = end + 1;
	}
	else {
		end = PROC_MAX;
	}
	
	while(1) {
		if(i == end)
			break;

		if(i >= PROC_MAX)
			i = 0;
		if(_proc_table[i].state == READY)
			return &_proc_table[i];
		i++;
	}
	return NULL;
}

static inline  uint32_t proc_get_user_stack(proc_t* proc) {
	(void)proc;
  return USER_STACK_BOTTOM;
}

static void* proc_get_mem_tail(void* p) {
	proc_t* proc = (proc_t*)p;
	return (void*)proc->space->heap_size;
}

static int32_t proc_expand(void* p, int32_t page_num) {
	return proc_expand_mem((proc_t*)p, page_num);
}

static void proc_shrink(void* p, int32_t page_num) {
	proc_shrink_mem((proc_t*)p, page_num);
}

static void proc_init_space(proc_t* proc) {
	page_dir_entry_t *vm = _proc_vm[proc->pid];
	set_kernel_vm(vm);
	proc->space = (proc_space_t*)km_alloc(sizeof(proc_space_t));
	proc->space->vm = vm;
	proc->space->heap_size = 0;
	
	proc->space->malloc_man.arg = (void*)proc;
	proc->space->malloc_man.head = 0;
	proc->space->malloc_man.tail = 0;
	proc->space->malloc_man.expand = proc_expand;
	proc->space->malloc_man.shrink = proc_shrink;
	proc->space->malloc_man.get_mem_tail = proc_get_mem_tail;
}

void proc_switch(context_t* ctx, proc_t* to){
	if(to == NULL || to == _current_proc)
		return;

	if(_current_proc != NULL) {
		memcpy(&_current_proc->ctx, ctx, sizeof(context_t));
	}

	_current_proc = to;
	memcpy(ctx, &_current_proc->ctx, sizeof(context_t));

	page_dir_entry_t *vm = _current_proc->space->vm;
	__set_translation_table_base((uint32_t) V2P(vm));
	__asm__ volatile("push {R4}");
	__asm__ volatile("mov R4, #0");
	__asm__ volatile("MCR p15, 0, R4, c8, c7, 0");
	__asm__ volatile("pop {R4}");
}

/* proc_exapnad_memory expands the heap size of the given process. */
int32_t proc_expand_mem(proc_t *proc, int32_t page_num) {
	int32_t i;
	for (i = 0; i < page_num; i++) {
		char *page = kalloc();
		if(page == NULL) {
			printf("proc expand failed!! free mem size: (%x), pid:%d, pages ask:%d\n", get_free_mem_size(), proc->pid, page_num);
			//proc_shrink_mem(proc, i);
			return -1;
		}
		memset(page, 0, PAGE_SIZE);
		map_page(proc->space->vm,
				proc->space->heap_size,
				V2P(page),
				AP_RW_RW);
		proc->space->heap_size += PAGE_SIZE;
	}
	return 0;
}

/* proc_shrink_memory shrinks the heap size of the given process. */
void proc_shrink_mem(proc_t* proc, int32_t page_num) {
	int32_t i;
	for (i = 0; i < page_num; i++) {
		uint32_t virtual_addr = proc->space->heap_size - PAGE_SIZE;
		uint32_t physical_addr = resolve_phy_address(proc->space->vm, virtual_addr);
		//get the kernel address for kalloc/kfree
		uint32_t kernel_addr = P2V(physical_addr);
		kfree((void *) kernel_addr);

		unmap_page(proc->space->vm, virtual_addr);
		proc->space->heap_size -= PAGE_SIZE;
		if (proc->space->heap_size == 0)
			break;
	}
}

static void proc_free_space(proc_t *proc) {
	/*free file info*/
	proc_shrink_mem(proc, proc->space->heap_size / PAGE_SIZE);
	free_page_tables(proc->space->vm);
	km_free(proc->space);
}

/* proc_free frees all resources allocated by proc. */
void proc_exit(context_t* ctx, proc_t *proc, int32_t res) {
	(void)res;
	proc->state = UNUSED;
	kfree((void*)proc->user_stack);
	proc_free_space(proc);
	_current_proc = NULL;	
	schedule(ctx);
}

void* proc_malloc(proc_t *proc, uint32_t size) {
	if(proc == NULL || size == 0)
		return NULL;
	return trunk_malloc(&proc->space->malloc_man, size);
}

void proc_free(proc_t* proc, void* p) {
	if(proc == NULL || p == NULL)
		return;
	trunk_free(&proc->space->malloc_man, p);
}

/* proc_creates allocates a new process and returns it. */
proc_t *proc_create(void) {
	int32_t index = -1;
	int32_t i;
	for (i = 0; i < PROC_MAX; i++) {
		if (_proc_table[i].state == UNUSED) {
			index = i;
			break;
		}
	}
	if (index < 0)
		return NULL;

	proc_t *proc = &_proc_table[index];
	memset(proc, 0, sizeof(proc_t));
	proc->pid = index;

	proc->father_pid = -1;
	proc->state = CREATED;
	proc_init_space(proc);

	char *stack = kalloc();
  uint32_t user_stack =  proc_get_user_stack(proc);
  map_page(proc->space->vm,
		user_stack,
    V2P(stack),
    AP_RW_RW);
  proc->user_stack = user_stack;
	proc->ctx.sp = ((uint32_t)proc->user_stack)+PAGE_SIZE;
	proc->ctx.cpsr = 0x50;
	return proc;
}

/* proc_load loads the given ELF process image into the given process. */
int32_t proc_load_elf(proc_t *proc, const char *proc_image) {
	uint32_t prog_header_offset = 0;
	uint32_t prog_header_count = 0;
	uint32_t i = 0;

	proc_shrink_mem(proc, proc->space->heap_size/PAGE_SIZE);

	/*read elf format from saved proc image*/
	struct elf_header *header = (struct elf_header *) proc_image;
	if (header->type != ELFTYPE_EXECUTABLE)
		return -1;

	prog_header_offset = header->phoff;
	prog_header_count = header->phnum;

	for (i = 0; i < prog_header_count; i++) {
		uint32_t j = 0;
		struct elf_program_header *header = (void *) (proc_image + prog_header_offset);
		/* make enough room for this section */
		while (proc->space->heap_size < header->vaddr + header->memsz) {
			if(proc_expand_mem(proc, 1) != 0){ 
				return -1;
			}
		}
		/* copy the section from kernel to proc mem space*/
		uint32_t hvaddr = header->vaddr;
		uint32_t hoff = header->off;
		for (j = 0; j < header->memsz; j++) {
			uint32_t vaddr = hvaddr + j; /*vaddr in elf (proc vaddr)*/
			uint32_t paddr = resolve_phy_address(proc->space->vm, vaddr); /*trans to phyaddr by proc's page dir*/
			uint32_t vkaddr = P2V(paddr); /*trans the phyaddr to vaddr now in kernel page dir*/
			/*copy from elf to vaddrKernel(=phyaddr=vaddrProc=vaddrElf)*/

			uint32_t image_off = hoff + j;
			*(char*)vkaddr = proc_image[image_off];
		}
		prog_header_offset += sizeof(struct elf_program_header);
	}

	proc->space->malloc_man.head = 0;
	proc->space->malloc_man.tail = 0;
	proc->ctx.pc = header->entry;
	proc->state = READY;
	return 0;
}
