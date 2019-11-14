#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <vprintf.h>
#include <shm.h>
#include <dev/fbinfo.h>
#include <x/xclient.h>

static int run(void) {
	int fd = open("/dev/keyb0", O_RDONLY);
	if(fd < 0)
		return -1;

	x_t* x = x_open(100, 100, 800, 600);
	if(x == NULL) {
		close(fd);
		return -1;
	}
	graph_t* g = x_graph(x);

	console_t console;
	console_init(&console);
	console.g = g;

	const char* fnt_name = getenv("font");
	if(fnt_name[0] == 0)
		fnt_name = "9x16";
	console.font = get_font_by_name(fnt_name);

	console.fg_color = 0xffffffff;
	console.bg_color = 0xff000000;
	console_reset(&console);

	int rd = 0;
	char c = 0;
	while(1) {
		if(rd != 1)
			rd = read(fd, &c, 1);
		else {
			if(write_nblock(1, &c, 1) == 1)
				rd = 0;
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
		x_update(x);
	}

	close(fd);
	console_close(&console);
	x_close(x);
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
