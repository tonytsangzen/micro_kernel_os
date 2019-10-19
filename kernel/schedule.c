#include <kernel/proc.h>

void schedule(context_t* ctx) {
	int32_t next = proc_get_next_ready();
	proc_switch(ctx, next);
}
