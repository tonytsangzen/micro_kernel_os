#include <stdio.h>
#include <vprintf.h>
#include <unistd.h>
#include <string.h>

static void outc(char c, void* p) {
	(void)p;
	write(1, &c, 1);
}

void dprintf(int fd, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	v_printf(outc, &fd, format, ap);
	va_end(ap);
}

void printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	v_printf(outc, NULL, format, ap);
	va_end(ap);
}

int getch(void) {
	while(1) {
		char c;
		int i = read(0, &c, 1);
		if(i == 1)
			return c;

		if(i < 0)
			break;
	}
	return -1;
}

void putch(int c) {
	write(1, &c, 1);
}

