#include <kernel/proc.h>

void schedule(context_t* ctx) {
	proc_t* next = proc_get_next_ready();
	if(next == NULL)
		return;
	proc_switch(ctx, next);
}
