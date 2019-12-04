#include <stdio.h>
#include <vprintf.h>
#include <unistd.h>
#include <string.h>
#include <mstr.h>
#include <syscall.h>
#include <dev/device.h>

static void outc(char c, void* p) {
	str_t* buf = (str_t*)p;
	str_addc(buf, c);
}

void dprintf(int fd, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	str_t* buf = str_new("");
	v_printf(outc, buf, format, ap);
	va_end(ap);
	write(fd, buf->cstr, buf->len);
	str_free(buf);
}

void printf(const char *format, ...) {
	va_list ap;
	str_t* buf = str_new("");
	va_start(ap, format);
	v_printf(outc, buf, format, ap);
	va_end(ap);
	write(1, buf->cstr, buf->len);
	str_free(buf);
}

void uprintf(const char *format, ...) {
	va_list ap;
	str_t* buf = str_new("");
	va_start(ap, format);
	v_printf(outc, buf, format, ap);
	va_end(ap);
	syscall3(SYS_DEV_CHAR_WRITE, DEV_UART0, (int32_t)buf->cstr, (int32_t)buf->len);
	str_free(buf);
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

