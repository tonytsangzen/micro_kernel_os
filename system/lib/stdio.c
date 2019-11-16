#include <stdio.h>
#include <vprintf.h>
#include <unistd.h>
#include <string.h>
#include <tstr.h>
#include <syscall.h>
#include <dev/device.h>

static void outc(char c, void* p) {
	tstr_t* buf = (tstr_t*)p;
	tstr_addc(buf, c);
}

void dprintf(int fd, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	tstr_t* buf = tstr_new("");
	v_printf(outc, buf, format, ap);
	va_end(ap);
	write(fd, buf->items, buf->size);
	tstr_free(buf);
}

void printf(const char *format, ...) {
	va_list ap;
	tstr_t* buf = tstr_new("");
	va_start(ap, format);
	v_printf(outc, buf, format, ap);
	va_end(ap);
	write(1, buf->items, buf->size);
	tstr_free(buf);
}

void uprintf(const char *format, ...) {
	va_list ap;
	tstr_t* buf = tstr_new("");
	va_start(ap, format);
	v_printf(outc, buf, format, ap);
	va_end(ap);
	syscall3(SYS_DEV_WRITE, DEV_UART0, (int32_t)buf->items, (int32_t)buf->size);
	tstr_free(buf);
}

int getch(void) {
	while(1) {
		char c;
		int i = read(0, &c, 1);
		if(i == 1) {
			return c;
		}
		if(i <= 0 && errno != EAGAIN)
			break;
		sleep(0);
	}
	return 0;
}

void putch(int c) {
	write(1, &c, 1);
}

