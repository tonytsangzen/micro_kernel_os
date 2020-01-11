#include <procinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <sysinfo.h>
#include <string.h>
#include <vprintf.h>
#include <cmain.h>
#include <mstr.h>
#include <console.h>

typedef struct {
	int lcd_fd;
	console_t console;
} shell_t;

static shell_t _shell;

static void lcd_init(void) {
	int fd = open("/dev/lcd", O_RDWR);
	if(fd < 0)
		return;

	proto_t out;	
	proto_init(&out, NULL, 0);
	int res = fcntl_raw(fd, CNTL_INFO, NULL, &out);
	if(res != 0) {
		close(fd);
		return;
	}
	int w = proto_read_int(&out);
	int h = proto_read_int(&out);
	proto_clear(&out);
	
	_shell.lcd_fd = fd;
	console_init(&_shell.console);
	graph_t* g = graph_new(NULL, w, h);
	_shell.console.g = g;
	_shell.console.fg_color = argb(0xff, 0xff, 0xff, 0x00);
	_shell.console.font = font_by_name("12x24");
	console_reset(&_shell.console);
}

static void lcd_close(void) {
	if(_shell.console.g == NULL)
		return;
	graph_free(_shell.console.g);
	console_close(&_shell.console);
	close(_shell.lcd_fd);
}

static void outc(char c, void* p) {
	str_t* buf = (str_t*)p;
	str_addc(buf, c);
}

static void lcd_flush(void) {
	write(_shell.lcd_fd, _shell.console.g->buffer, _shell.console.g->w*_shell.console.g->h*4);
}

static void outs(const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	str_t* buf = str_new("");
	v_printf(outc, buf, format, ap);
	va_end(ap);
	if(_shell.console.g != NULL) {
		console_put_string(&_shell.console, buf->cstr);
		lcd_flush();
	}
	str_free(buf);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	int joy_fd = open("/dev/joystick", O_RDONLY);
	if(joy_fd < 0)
		return -1;
	lcd_init();
	const char* prompt = ""
			"================\n"
			"joyshell V0.1   \n"
			"----------------";

	console_clear(&_shell.console);
	outs("%s\n", prompt);

	char old_key = 0;
	while(true) {
		char key;
		read(joy_fd, &key, 1);
		if(key != old_key) {
			console_clear(&_shell.console);
			outs("%s\nkey: %d\n", prompt, key);
		}
		old_key = key;
	}

	close(joy_fd);
	lcd_close();

	return 0;
}
