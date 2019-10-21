#include <stdlib.h>
#include <svc_call.h>

void *malloc(size_t size) {
	return (void*)svc_call1(SYS_MALLOC, (int32_t)size);
}

void free(void* ptr) {
	svc_call1(SYS_FREE, (int32_t)ptr);
}

void exit(int status) {
	svc_call1(SYS_EXIT, (int32_t)status);
}
