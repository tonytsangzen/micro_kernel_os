#include <kernel/proc.h>
#include <kernel/system.h>

void schedule(context_t* ctx) {
	proc_t* next = NULL;
	if(_current_proc == NULL) 
		next = _ready_proc;
	else 
		next = _current_proc->next;

	if(next != NULL)
		proc_switch(ctx, next);
}
