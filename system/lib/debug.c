#include <debug.h>
#include <svc_call.h>
#include <dev/device.h>
#include <vprintf.h>
#include <string.h>

void uart_debug(const char* str) {
	svc_call3(SYS_DEV_WRITE, DEV_UART0, (int32_t)str, strlen(str));
}

static void outc(char c, void* p) {
	(void)p;
	char s[2] = {c, 0};
	uart_debug(s);
}

void debug(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	v_printf(outc, 0, format, ap);
	va_end(ap);
}
