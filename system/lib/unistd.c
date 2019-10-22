#include <unistd.h>
#include <svc_call.h>

int getpid(void) {
	return svc_call0(SYS_GET_PID);
}

int fork(void) {
	return svc_call0(SYS_FORK);
}
