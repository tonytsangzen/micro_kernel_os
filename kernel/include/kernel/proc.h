#ifndef PROC_H
#define PROC_H

#include <kernel/context.h>
#include <mm/mmu.h>
#include <mm/trunkmalloc.h>

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
	page_dir_entry_t *vm;
	malloc_t malloc_man;
	uint32_t heap_size;
} proc_space_t;

typedef struct {
	int32_t pid;
	int32_t father_pid;
	int32_t state;
	context_t ctx;

	proc_space_t* space;
	uint32_t user_stack;
} proc_t;

extern proc_t* _current_proc;

extern void    procs_init(void);
extern proc_t* proc_create(void);
extern int32_t proc_load_elf(proc_t *proc, const char *proc_image);
extern int32_t proc_start(proc_t* proc, uint32_t entry);
extern int32_t proc_get_next_ready(void);
extern void    proc_switch(context_t* ctx, int32_t pid);
extern int32_t proc_expand_mem(proc_t *proc, int32_t page_num);
extern void    proc_shrink_mem(proc_t* proc, int32_t page_num);
extern void    proc_exit(proc_t *proc);

extern void*   proc_malloc(uint32_t size);
extern void    proc_free(void* p);

#define PROC_MAX 128

#endif
