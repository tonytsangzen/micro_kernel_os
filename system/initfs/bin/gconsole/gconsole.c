#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <vprintf.h>

static int run(void) {
	int fd = open("/dev/keyb0", O_RDONLY);
	if(fd < 0)
		return -1;

	console_t console;
	console_init(&console);
	console.g = graph_from_fb();

	const char* fnt_name = getenv("font");
	if(fnt_name[0] == 0)
		fnt_name = "9x16";
	console.font = get_font_by_name(fnt_name);

	console.fg_color = 0xffffffff;
	console.bg_color = 0xff000000;
	console_reset(&console);

	while(1) {
		char c;
		if(read(fd, &c, 1) == 1) {
			write(1, &c, 1);
		}

		char buf[256];
		int32_t size = read(0, buf, 255);
		if(size == 0) {
			break;
		}
		else if(size < 0) {
			if(errno == EAGAIN) {
				sleep(0);
				continue;
			}
			else 
				break;
		}

		buf[size] = 0;
		const char* p = (const char*)buf;
		for(int32_t i=0; i<size; i++) {
			char c = p[i];
			console_put_char(&console, c);
		}
		graph_flush_fb(console.g);
	}

	close(fd);
	console_close(&console);
	return 0;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int fds1[2];
	int fds2[2];
	pipe(fds1);
	pipe(fds2);

	int pid = fork();
	if(pid != 0) { //father proc for p2 reader.
		dup2(fds1[0], 0);
		dup2(fds2[1], 1);
		close(fds1[0]);
		close(fds1[1]);
		close(fds2[0]);
		close(fds2[1]);
		return run();
	}
	//child proc for p1 writer
	dup2(fds1[1], 1);
	dup2(fds2[0], 0);
	close(fds1[0]);
	close(fds1[1]);
	close(fds2[0]);
	close(fds2[1]);
	return exec("/initrd/bin/shell");
}
