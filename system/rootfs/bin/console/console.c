#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <vprintf.h>
#include <shm.h>
#include <sconf.h>
#include <global.h>
#include <dev/fbinfo.h>

typedef struct {
	int fb_fd;
	int shm_id;
	console_t console;
} fb_console_t;

static void init_console(fb_console_t* console) {
	int fb_fd = open("/dev/fb0", O_RDONLY);
	if(fb_fd < 0) {
		return;
	}

	int id = dma(fb_fd, NULL);
	if(id <= 0) {
		close(fb_fd);
		return;
	}

	void* gbuf = shm_map(id);
	if(gbuf == NULL) {
		close(fb_fd);
		return;
	}

	fbinfo_t info;
	proto_t out;
	proto_init(&out, NULL, 0);

	if(cntl_raw(fb_fd, CNTL_INFO, NULL, &out) != 0) {
		shm_unmap(id);
		close(fb_fd);
		return;
	}

	proto_read_to(&out, &info, sizeof(fbinfo_t));
	graph_t* g = graph_new(gbuf, info.width, info.height);
	proto_clear(&out);

	console->fb_fd = fb_fd;
	console->shm_id = id;
	console_init(&console->console);
	console->console.g = g;
	console->console.font = font_by_name("8x16");
	console->console.fg_color = 0xffffffff;
	console->console.bg_color = 0xff000000;
	console_reset(&console->console);
}

static void close_console(fb_console_t* console) {
	graph_free(console->console.g);
	shm_unmap(console->shm_id);
	close(console->fb_fd);
}

static int run(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int fd = open("/dev/keyb0", O_RDONLY);
	if(fd < 0)
		return -1;

	fb_console_t console;
	init_console(&console);

	int actived = 0;
	while(1) {
		const char* cc = get_global("current_console");
		if(cc[0] == 'c') {
			if(actived == 0) {
				console_refresh(&console.console);
				flush(console.fb_fd);
			}
			actived = 1;
		}
		else {
			actived = 0;
			sleep(0);
			continue;
		}

		int8_t c;
		//read keyb
		int rd = read(fd, &c, 1);
		if(rd == 1) {
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
			console_put_char(&console.console, c);
		}
		flush(console.fb_fd);
	}

	close(fd);
	close_console(&console);
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
		return run(argc, argv);
	}
	//child proc for p1 writer
	dup2(fds1[1], 1);
	dup2(fds2[0], 0);
	close(fds1[0]);
	close(fds1[1]);
	close(fds2[0]);
	close(fds2[1]);
	setenv("CONSOLE", "console");

	exec("/bin/session");
	return 0;
}
