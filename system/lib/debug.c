#include <debug.h>
#include <svc_call.h>
#include <vprintf.h>

void uart_debug(const char* str) {
	svc_call1(SYS_UART_DEBUG, (int32_t)str);
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
