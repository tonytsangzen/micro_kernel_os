#include <kernel/system.h>
#include <kernel/proc.h>
#include <vfs.h>
#include <kernel/kernel.h>
#include <kernel/schedule.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <kprintf.h>
#include <elf.h>

static proc_t _proc_table[PROC_MAX];
__attribute__((__aligned__(PAGE_DIR_SIZE))) 
static page_dir_entry_t _proc_vm[PROC_MAX][PAGE_DIR_NUM];
proc_t* _current_proc = NULL;
proc_t* _ready_proc = NULL;

/* proc_init initializes the process sub-system. */
void procs_init(void) {
	int32_t i;
	for (i = 0; i < PROC_MAX; i++)
		_proc_table[i].state = UNUSED;
	_current_proc = NULL;
	_ready_proc = NULL;
}

proc_t* proc_get(int32_t pid) {
	if(pid < 0 || pid >= PROC_MAX)
		return NULL;
	return &_proc_table[pid];
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
	proc->space = (proc_space_t*)kmalloc(sizeof(proc_space_t));
	memset(proc->space, 0, sizeof(proc_space_t));

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
	if(to == NULL)
		return;

	if(_current_proc != NULL) {
		memcpy(&_current_proc->ctx, ctx, sizeof(context_t));
	}

	memcpy(ctx, &to->ctx, sizeof(context_t));

	if(_current_proc != to) {
		page_dir_entry_t *vm = to->space->vm;
		__set_translation_table_base((uint32_t) V2P(vm));
		_current_proc = to;
	}
}

/* proc_exapnad_memory expands the heap size of the given process. */
int32_t proc_expand_mem(proc_t *proc, int32_t page_num) {
	int32_t i;
	for (i = 0; i < page_num; i++) {
		char *page = kalloc4k();
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
		uint32_t kernel_addr = resolve_kernel_address(proc->space->vm, virtual_addr);
		//get the kernel address for kalloc4k/kfree4k
		kfree4k((void *) kernel_addr);

		unmap_page(proc->space->vm, virtual_addr);
		proc->space->heap_size -= PAGE_SIZE;
		if (proc->space->heap_size == 0)
			break;
	}
}

static void proc_close_files(proc_t *proc) {
	int32_t i;
	for(i=0; i<PROC_FILE_MAX; i++) {
		if(proc->space->files[i].node != 0)
			vfs_close_raw(proc->pid, i);
	}
}

static void proc_free_space(proc_t *proc) {
	/*close files*/
	proc_close_files(proc);

	/*free file info*/
	proc_shrink_mem(proc, proc->space->heap_size / PAGE_SIZE);
	free_page_tables(proc->space->vm);
	kfree(proc->space);
}

static void proc_ready(proc_t* proc) {
	if(proc == NULL || proc->state == READY)
		return;

	proc->state = READY;
	if(_ready_proc == NULL)
		_ready_proc = proc;

	proc->next = _ready_proc;
	proc->prev = _ready_proc->prev;
	_ready_proc->prev->next = proc;
	_ready_proc->prev = proc;
}

static void proc_unready(context_t* ctx, proc_t* proc) {
	if(proc == NULL)
		return;

	if(proc->next != NULL)
		proc->next->prev = proc->prev;
	if(proc->prev != NULL)
		proc->prev->next = proc->next;

	if(proc->next == proc) //only one left.
		_ready_proc = NULL;
	else
		_ready_proc = proc->next;

	proc->next = NULL;
	proc->prev = NULL;

	if(_current_proc == proc) {
		memcpy(&_current_proc->ctx, ctx, sizeof(context_t));
		_current_proc = NULL;
		schedule(ctx);
	}
}

static void proc_wakeup_waiting(int32_t pid) {
	int32_t i;
	for (i = 0; i < PROC_MAX; i++) {
		proc_t *proc = &_proc_table[i];
		if (proc->state == WAIT && proc->wait_pid == pid) {
			proc_ready(proc);
		}
	}
}

/* proc_free frees all resources allocated by proc. */
void proc_exit(context_t* ctx, proc_t *proc, int32_t res) {
	(void)res;
	int32_t pid = proc->pid;

	uint32_t cpsr = __int_off();

	uint32_t kernel_addr = resolve_kernel_address(proc->space->vm, proc->user_stack);
	kfree4k((void *) kernel_addr);
	proc_free_space(proc);
	proc_wakeup_waiting(pid);
	proc->state = UNUSED;
	proc_unready(ctx, proc);
	memset(proc, 0, sizeof(proc_t));

	__int_on(cpsr);
}

void* proc_malloc(uint32_t size) {
	if(size == 0)
		return NULL;
	return trunk_malloc(&_current_proc->space->malloc_man, size);
}

void proc_free(void* p) {
	if(p == NULL)
		return;
	trunk_free(&_current_proc->space->malloc_man, p);
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

	char *stack = kalloc4k();
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
			uint32_t vkaddr = resolve_kernel_address(proc->space->vm, vaddr); /*trans to phyaddr by proc's page dir*/
			/*copy from elf to vaddrKernel(=phyaddr=vaddrProc=vaddrElf)*/

			uint32_t image_off = hoff + j;
			*(char*)vkaddr = proc_image[image_off];
		}
		prog_header_offset += sizeof(struct elf_program_header);
	}

	proc->space->malloc_man.head = 0;
	proc->space->malloc_man.tail = 0;
	proc->ctx.sp = ((uint32_t)proc->user_stack)+PAGE_SIZE;
	proc->ctx.pc = header->entry;
	proc->ctx.lr = header->entry;
	proc_ready(proc);
	return 0;
}

void proc_sleep_on(context_t* ctx, uint32_t event) {
	if(_current_proc == NULL)
		return;

	uint32_t cpsr = __int_off();

	_current_proc->sleep_event = event;
	_current_proc->state = SLEEPING;
	proc_unready(ctx, _current_proc);

	__int_on(cpsr);
}

void proc_waitpid(context_t* ctx, int32_t pid) {
	if(_current_proc == NULL || _proc_table[pid].state == UNUSED)
		return;

	uint32_t cpsr = __int_off();

	_current_proc->wait_pid = pid;
	_current_proc->state = WAIT;
	proc_unready(ctx, _current_proc);

	__int_on(cpsr);
}

void proc_wakeup(uint32_t event) {
	uint32_t cpsr = __int_off();

	int32_t i = 0;	
	while(1) {
		if(i >= PROC_MAX)
			break;
		proc_t* proc = &_proc_table[i];	
		if(proc->state == SLEEPING && proc->sleep_event == event)
			proc_ready(proc);
		i++;
	}
	__int_on(cpsr);
}

static void proc_page_clone(proc_t* to, uint32_t to_addr, proc_t* from, uint32_t from_addr) {
	char *to_ptr = (char*)resolve_kernel_address(to->space->vm, to_addr);
	char *from_ptr = (char*)resolve_kernel_address(from->space->vm, from_addr);
	memcpy(to_ptr, from_ptr, PAGE_SIZE);
}

static int32_t proc_clone(proc_t* child, proc_t* parent) {
	uint32_t pages = parent->space->heap_size / PAGE_SIZE;
	if((parent->space->heap_size % PAGE_SIZE) != 0)
		pages++;

	uint32_t p;
	for(p=0; p<pages; ++p) {
		uint32_t v_addr = (p * PAGE_SIZE);
		/*
		page_table_entry_t * pge = get_page_table_entry(parent->space->vm, v_addr);
		if(pge->permissions == AP_RW_R) {
			uint32_t phy_page_addr = resolve_phy_address(parent->space->vm, v_addr);
			map_page(child->space->vm, 
					child->space->heap_size,
					phy_page_addr,
					AP_RW_R);
			child->space->heap_size += PAGE_SIZE;
		}
		*/
		//else {
			if(proc_expand_mem(child, 1) != 0) {
				printf("Panic: kfork expand memory failed!!(%d)\n", parent->pid);
				return -1;
			}
			// copy parent's memory to child's memory
			proc_page_clone(child, v_addr, parent, v_addr);
		//}
	}

	/*set father*/
	child->father_pid = parent->pid;
	/* copy parent's stack to child's stack */
	proc_page_clone(child, child->user_stack, parent, parent->user_stack);
	return 0;
}

proc_t* kfork() {
	proc_t *child = NULL;
	proc_t *parent = _current_proc;

	child = proc_create();
	if(child == NULL) {
		printf("panic: kfork create proc failed!!(%d)\n", parent->pid);
		return NULL;
	}

	if(proc_clone(child, parent) != 0) {
		printf("panic: kfork clone failed!!(%d)\n", parent->pid);
		return NULL;
	}

	proc_ready(child);
	return child;
}

int32_t proc_send_msg(int32_t to_pid, void* data, uint32_t size) {
	uint32_t cpsr = __int_off();

	proc_t* proc_to = proc_get(to_pid);
	if(proc_to == NULL || proc_to->state == UNUSED) {
		__int_on(cpsr);
		return -1;
	}

	proc_msg_t* msg = (proc_msg_t*)kmalloc(sizeof(proc_msg_t));
	if(msg == NULL) {
		__int_on(cpsr);
		return -1;
	}
	
	msg->data = kmalloc(size);
	if(msg->data == NULL) {
		kfree(msg->data);
		__int_on(cpsr);
		return -1;
	}
	memcpy(msg->data, data, size);

	msg->from_pid = _current_proc->pid;
	msg->size = size;
	msg->next = NULL;

	
	if(proc_to->msg_queue_tail == NULL) {
		proc_to->msg_queue_tail = proc_to->msg_queue_head = msg;
	}
	else {
		proc_to->msg_queue_tail->next = msg;
		proc_to->msg_queue_tail = msg;
	}
	__int_on(cpsr);

	proc_wakeup((uint32_t)&proc_to->pid);
	return 0;
}

void* proc_get_msg(context_t* ctx, int32_t *pid, uint32_t* size, int32_t block) {
	void *ret = NULL;
	uint32_t cpsr = __int_off();

	proc_msg_t* msg = _current_proc->msg_queue_head;
	if(msg != NULL) {
		if(msg->next == NULL)
			_current_proc->msg_queue_tail = NULL;
		_current_proc->msg_queue_head = msg->next;

		if(pid != NULL)
			*pid = msg->from_pid;
		if(size != NULL)
			*size = msg->size;

		ret = proc_malloc(msg->size);
		if(ret != NULL) 
			memcpy(ret, msg->data, msg->size);

		kfree(msg->data);
		kfree(msg);
		__int_on(cpsr);
	}
	else {
		__int_on(cpsr);
		if(block != 0) {
			ctx->gpr[0] = 0;
			proc_sleep_on(ctx, (uint32_t)&_current_proc->pid);
		}
	}

	return ret;
}

