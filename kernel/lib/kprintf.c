#include "kprintf.h"
#include "vprintf.h"
#include "dev/uart.h"
#include "tstr.h"

static void outc(char c, void* p) {
	tstr_t* buf = (tstr_t*)p;
	tstr_addc(buf, c);
}

void printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	tstr_t* buf = tstr_new("");
	v_printf(outc, buf, format, ap);
	va_end(ap);
	uart_write(NULL, buf->items, buf->size);
	tstr_free(buf);
}
