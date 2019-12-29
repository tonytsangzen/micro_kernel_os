#include "kprintf.h"
#include "vprintf.h"
#include "dev/uart.h"
#include "kstring.h"

static console_t _console;

static void outc(char c, void* p) {
	(void)p;
		
	uart_write(NULL, &c, 1);
	if(_console.g != NULL)
		console_put_char(&_console, c);
}

void uart_out(const char* s) {
	if(get_dev(DEV_UART0)->state == DEV_STATE_OFF)
		return;
	uart_write(NULL, s, strlen(s));
	if(_console.g != NULL)
		console_put_string(&_console, s);
}

void printf(const char *format, ...) {
	if(get_dev(DEV_UART0)->state == DEV_STATE_OFF)
		return;

	va_list ap;
	va_start(ap, format);
	v_printf(outc, NULL, format, ap);
	va_end(ap);
}

inline console_t* get_console(void) {
	return &_console;
}
