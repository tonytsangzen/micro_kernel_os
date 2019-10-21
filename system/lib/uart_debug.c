#include <uart_debug.h>
#include <svc_call.h>

void uart_debug(const char* str) {
	svc_call1(SYS_UART_DEBUG, (int32_t)str);
}
