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

int execl(const char* fname, const char* arg, ...) {
	(void)fname; //TODO
	(void)arg; //TODO

	return 0;
}

const char* getenv(const char* name) {
	static char ret[1024];
	svc_call3(SYS_PROC_GET_ENV, (int32_t)name, (int32_t)ret, 1023);
	return ret;
}

int setenv(const char* name, const char* value) {
	return svc_call2(SYS_PROC_SET_ENV, (int32_t)name, (int32_t)value);
}
