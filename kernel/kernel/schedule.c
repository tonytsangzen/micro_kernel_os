#include <kernel/proc.h>
#include <kernel/system.h>
#include <kprintf.h>

void schedule(context_t* ctx) {
	uint32_t cpsr = __int_off();

	proc_t* next = NULL;
	if(_current_proc == NULL || _current_proc->state != READY) 
		next = _ready_proc;
	else 
		next = _current_proc->next;

	if(next != NULL) {
		proc_switch(ctx, next);
	}
	else {
		printf("dead schedule!\n");
		while(1);
	}
	__int_on(cpsr);
}
