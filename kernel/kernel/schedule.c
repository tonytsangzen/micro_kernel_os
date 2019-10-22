#include <kernel/proc.h>
#include <kernel/system.h>

void schedule(context_t* ctx) {
	uint32_t cpsr = __int_off();	

	proc_t* next = proc_get_next_ready();
	if(next != NULL)
		proc_switch(ctx, next);

	__int_on(cpsr);	
}
