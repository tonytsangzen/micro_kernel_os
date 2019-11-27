#include "kprintf.h"
#include "vprintf.h"
#include "dev/uart.h"
#include "tstr.h"
#include "kstring.h"

static void outc(char c, void* p) {
	tstr_t* buf = (tstr_t*)p;
	tstr_addc(buf, c);
}

void uart_out(const char* s) {
	uart_write(NULL, s, strlen(s));
}

void printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	tstr_t* buf = tstr_new("");
	v_printf(outc, buf, format, ap);
	va_end(ap);
	tstr_addc(buf, 0);
	uart_out(buf->items);
	tstr_free(buf);
}
