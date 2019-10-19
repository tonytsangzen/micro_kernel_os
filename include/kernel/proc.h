#ifndef PROC_H
#define PROC_H

#include <kernel/context.h>
#include <mm/mmu.h>

enum {
	UNUSED = 0,
	CREATED,
	SLEEPING,
	WAIT,
	BLOCK,
	READY,
	RUNNING,
	TERMINATED
};

typedef struct {
	int32_t pid;
	int32_t father_pid;
	int32_t state;
	context_t ctx;
	char stack[PAGE_SIZE];
} proc_t;

extern proc_t* _current_proc;

extern void proc_init(void);
extern int32_t proc_get_next_ready(void);
extern void proc_switch(context_t* ctx, int32_t pid);
extern int32_t proc_add(uint32_t entry);

#define PROC_MAX 128

#endif
