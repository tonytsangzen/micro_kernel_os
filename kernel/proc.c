#include <kernel/system.h>
#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <mm/kalloc.h>
#include <kstring.h>
#include <printk.h>

static proc_t _proc_table[PROC_MAX];
__attribute__((__aligned__(PAGE_DIR_SIZE))) 
static page_dir_entry_t _proc_vm[PROC_MAX][PAGE_DIR_NUM];
proc_t* _current_proc = NULL;

static page_dir_entry_t* proc_get_vm(proc_t* proc) {
	return _proc_vm[proc->pid];
}

/* proc_init initializes the process sub-system. */
void proc_init(void) {
	int32_t i;
	for (i = 0; i < PROC_MAX; i++)
		_proc_table[i].state = UNUSED;
	_current_proc = NULL;
}

int32_t proc_get_next_ready(void) {
	int32_t i = 0;
	if(_current_proc != NULL)
		i = _current_proc->pid + 1;
	
	while(1) {
		if(i >= PROC_MAX)
			i = 0;
		if(_proc_table[i].state == READY ||
				i == _current_proc->pid)
			break;
		i++;
	}
	return i;
}

static inline  uint32_t proc_get_user_stack(proc_t* proc) {
	(void)proc;
  return USER_STACK_BOTTOM;
}

/* proc_creates allocates a new process and returns it. */
static proc_t *proc_create(void) {
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

	page_dir_entry_t *vm = proc_get_vm(proc);
	set_kernel_vm(vm);

	char *stack = kalloc();
  uint32_t user_stack =  proc_get_user_stack(proc);
  map_page(vm,
		user_stack,
    V2P(stack),
    AP_RW_RW);
  proc->user_stack = user_stack;
	return proc;
}

int32_t proc_add(uint32_t entry) {
	proc_t *proc = proc_create();
	if(proc == NULL)
		return -1;
	proc->ctx.sp = ((uint32_t)proc->user_stack)+PAGE_SIZE;
	proc->ctx.cpsr = 0x50;
	proc->ctx.pc = entry;
	proc->state = READY;
	return proc->pid;
}

void proc_switch(context_t* ctx, int32_t pid){
	if(_current_proc != NULL) {
		memcpy(&_current_proc->ctx, ctx, sizeof(context_t));
		if(pid == _current_proc->pid)
			return;
	}

	_current_proc = &_proc_table[pid];
	memcpy(ctx, &_current_proc->ctx, sizeof(context_t));

	page_dir_entry_t *vm = proc_get_vm(_current_proc);
	__set_translation_table_base((uint32_t) V2P(vm));
	__asm__ volatile("push {R4}");
  __asm__ volatile("mov R4, #0");
  __asm__ volatile("MCR p15, 0, R4, c8, c7, 0");
  __asm__ volatile("pop {R4}");
}

