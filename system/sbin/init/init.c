#include <svc_call.h>
#include <syscalls.h>

void _start(void) {
	svc_call1(SYS_UART_DEBUG, (int32_t)"hello init.\n");
	svc_call1(SYS_EXIT, 0);
}
