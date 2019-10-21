#include "kprintf.h"
#include "vprintf.h"
#include "dev/uart_basic.h"

static void outc(char c, void* p) {
	(void)p;
	uart_basic_putch(c);
}

void printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	v_printf(outc, 0, format, ap);
	va_end(ap);
}
