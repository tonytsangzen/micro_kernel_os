#include "kprintf.h"
#include "vprintf.h"
#include "dev/uart.h"
#include "dev/framebuffer.h"
#include "kstring.h"
#include "mstr.h"
#include "graph.h"

static console_t _console;

void uart_out(const char* s) {
	uart_write(NULL, s, strlen(s));
}

static void outc(char c, void* p) {
	str_t* buf = (str_t*)p;
	str_addc(buf, c);
}

void printf(const char *format, ...) {
	if(get_dev(DEV_UART0)->state == DEV_STATE_OFF)
		return;

	va_list ap;
	va_start(ap, format);
	str_t* buf = str_new("");
	v_printf(outc, buf, format, ap);
	va_end(ap);
	
	uart_out(buf->cstr);
	if(_console.g != NULL) {
		fbinfo_t* info = fb_get_info();
		console_put_string(&_console, buf->cstr);
		if(info->depth == 16)
			dup16((uint16_t*)info->pointer, _console.g->buffer, info->width, info->height);
		else if(info->depth == 32)
			memcpy((void*)info->pointer, _console.g->buffer, info->size);
	}
	str_free(buf);
}

inline console_t* get_console(void) {
	return &_console;
}
