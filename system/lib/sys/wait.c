#include <svc_call.h>

int waitpid(int pid) {
	return svc_call1(SYS_WAIT_PID, pid);
}
