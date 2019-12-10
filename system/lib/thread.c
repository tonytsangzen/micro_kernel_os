#include <thread.h>
#include <stdlib.h>
#include <syscall.h>

typedef struct {
	thread_func_t func;
	void* p;
} thread_entry_t;

static void thread_entry(void* p) {
	thread_entry_t *entry = (thread_entry_t*)p;
	entry->func(entry->p);
	free(entry);
	exit(0);
}

int thread_create(thread_func_t func, void* p) {
	thread_entry_t *entry = (thread_entry_t*)malloc(sizeof(thread_entry_t));
	entry->func = func;
	entry->p = p;
	return syscall2(SYS_THREAD, (int32_t)thread_entry, (int32_t)entry);
}
