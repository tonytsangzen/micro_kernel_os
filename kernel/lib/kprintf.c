#include "kprintf.h"
#include "vprintf.h"
#include "dev/uart.h"
#include "kstring.h"

static void outc(char c, void* p) {
	(void)p;
	uart_write(NULL, &c, 1);
}

void uart_out(const char* s) {
	uart_write(NULL, s, strlen(s));
}

void printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	v_printf(outc, NULL, format, ap);
	va_end(ap);
}
