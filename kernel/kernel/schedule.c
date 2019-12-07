#include <kernel/proc.h>
#include <kernel/system.h>
#include <kprintf.h>

void schedule(context_t* ctx) {
	proc_t* next = NULL;
	if(_current_proc != NULL)
		next = _current_proc->next;
	if(next == NULL)
		next = _ready_proc;

	if(next != NULL) {
		proc_switch(ctx, next);
	}
	else {
		printf("dead schedule!\n");
		while(1);
	}
}
