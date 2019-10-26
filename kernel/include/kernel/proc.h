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

typedef struct st_proc_msg {
	int32_t from_pid;
	uint32_t size;
	void* data;
	struct st_proc_msg* next;
} proc_msg_t;

typedef struct st_proc {
	int32_t pid;
	int32_t father_pid;
	int32_t state;
	context_t ctx;

	uint32_t sleep_event;
	int32_t wait_pid;

	proc_space_t* space;
	uint32_t user_stack;

	proc_msg_t *msg_queue_head;
	proc_msg_t *msg_queue_tail;
	struct st_proc* prev;
	struct st_proc* next;
} proc_t;

extern proc_t* _current_proc;
extern proc_t* _ready_proc;

extern void    procs_init(void);
extern proc_t* proc_create(void);
extern int32_t proc_load_elf(proc_t *proc, const char *proc_image);
extern int32_t proc_start(proc_t* proc, uint32_t entry);
extern proc_t* proc_get_next_ready(void);
extern void    proc_switch(context_t* ctx, proc_t* to);
extern int32_t proc_expand_mem(proc_t *proc, int32_t page_num);
extern void    proc_shrink_mem(proc_t* proc, int32_t page_num);
extern void    proc_exit(context_t* ctx, proc_t *proc, int32_t res);

extern void*   proc_malloc(uint32_t size);
extern void    proc_free(void* p);

extern void    proc_sleep_on(context_t* ctx, uint32_t event);
extern void    proc_wakeup(uint32_t event);
extern void    proc_waitpid(context_t* ctx, int32_t pid);
extern proc_t* proc_get(int32_t pid);
extern proc_t* kfork(void);

extern int32_t proc_send_msg(int32_t to_pid, void* data, uint32_t size);
extern void*   proc_get_msg(context_t* ctx, int32_t *pid, uint32_t* size, uint8_t block);

#define PROC_MAX 128

#endif
