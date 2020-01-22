#include <kernel/uspace_int.h>
#include <kstring.h>

static uint32_t _uspace_int_table[USPACE_INT_MAX];

void uspace_interrupt_init(void) {
	for(int32_t i=0; i<USPACE_INT_MAX; i++) {
		_uspace_int_table[i] = 0;
	}
}

void proc_interrupt(context_t* ctx, int32_t pid, int32_t int_id) {
	proc_t* proc = proc_get(pid);	
	if(proc == NULL || proc->interrupt.entry == 0 || proc->interrupt.busy)
		return;

	proc_t *int_thread = kfork_raw(PROC_TYPE_INTERRUPT, proc);
	if(int_thread == NULL)
		return;

	uint32_t sp = int_thread->ctx.sp;
	memcpy(&int_thread->ctx, &proc->ctx, sizeof(context_t));
	int_thread->ctx.sp = sp;
	int_thread->ctx.pc = int_thread->ctx.lr = proc->interrupt.entry;
	int_thread->ctx.gpr[0] = int_id;
	int_thread->ctx.gpr[1] = proc->interrupt.func;
	int_thread->ctx.gpr[2] = proc->interrupt.data;
	proc->interrupt.busy = true;
	proc_switch(ctx, int_thread);
}

void uspace_interrupt(context_t* ctx, int32_t int_id) {
	if(int_id < 0 || int_id >= USPACE_INT_MAX)
		return;
	proc_interrupt(ctx, _uspace_int_table[int_id], int_id);
}

void uspace_interrupt_register(int32_t pid, int32_t int_id) {
	if(int_id < 0 || int_id >= USPACE_INT_MAX)
		return;
	_uspace_int_table[int_id] = pid;
}
