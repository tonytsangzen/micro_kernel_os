#include "kprintf.h"
#include "vprintf.h"
#include "dev/uart.h"
#include "dev/framebuffer.h"
#include "dev/actled.h"
#include "kstring.h"
#include "mstr.h"
#include "graph.h"
#include "kernel/system.h"

static console_t _console;

#ifdef WITH_LCDHAT
#include "lcdhat/lcdhat.h"
void init_console(void) {
	console_init(&_console);
	lcd_init();
	graph_t* g = graph_new(NULL, LCD_WIDTH, LCD_HEIGHT);
	_console.g = g;
//	_console.font = font_by_name("5x12");
	console_reset(&_console);
}

static void flush_console(void) {
	lcd_flush(_console.g->buffer, LCD_WIDTH*LCD_HEIGHT*4);
}

void setup_console(void) {
}

#else

void init_console(void) {
	console_init(&_console);
}

void setup_console(void) {
	if(_console.g != NULL) //already setup
		return;

	fbinfo_t* info = fb_get_info();
	if(info->pointer == 0)
		return;

	graph_t* g = graph_new(NULL, info->width, info->height);
	_console.g = g;
	console_reset(&_console);
}

static void flush_console(void) {
	fbinfo_t* info = fb_get_info();
	fb_dev_write(NULL, _console.g->buffer, info->width * info->height * 4);
}
#endif

void uart_out(const char* s) {
	uart_write(NULL, s, strlen(s));
}

static void outc(char c, void* p) {
	str_t* buf = (str_t*)p;
	str_addc(buf, c);
}

void flush_actled(void) {
	act_led(1);
	_delay(1000000);
	act_led(0);
	_delay(1000000);
}

void printf(const char *format, ...) {
	act_led(1);
	_delay_msec(10);

	va_list ap;
	va_start(ap, format);
	str_t* buf = str_new("");
	v_printf(outc, buf, format, ap);
	va_end(ap);
	
	uart_out(buf->cstr);
	act_led(0);
	if(_console.g != NULL) {
		console_put_string(&_console, buf->cstr);
		flush_console();
	}
	str_free(buf);
}

void close_console(void) {
	if(_console.g != NULL) {
		graph_free(_console.g);
		console_close(&_console);
	}
}

