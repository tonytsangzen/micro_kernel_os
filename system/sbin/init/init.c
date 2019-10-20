#include <svc_call.h>
#include <syscalls.h>

void _start(void) {
	while(1) {
		svc_call1(SYS_UART_DEBUG, (int32_t)"hello init.\n");
	}
}
