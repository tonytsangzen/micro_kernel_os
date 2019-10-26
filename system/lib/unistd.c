#include <unistd.h>
#include <svc_call.h>

int getpid(void) {
	return svc_call0(SYS_GET_PID);
}

int fork(void) {
	return svc_call0(SYS_FORK);
}

unsigned int sleep(unsigned int seconds) {
	if(seconds == 0)
		svc_call0(SYS_YIELD);
	return 0;
}
