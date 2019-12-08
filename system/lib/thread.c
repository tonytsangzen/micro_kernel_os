#include <thread.h>
#include <syscall.h>

int thread_create(thread_func_t func, void* p) {
	return syscall2(SYS_THREAD, (int32_t)func, (int32_t)p);
}
