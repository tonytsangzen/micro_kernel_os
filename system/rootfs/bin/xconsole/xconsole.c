#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <vprintf.h>
#include <shm.h>
#include <sconf.h>
#include <dev/fbinfo.h>
#include <x/xclient.h>

typedef struct {
	font_t* font;
	uint32_t fg_color;
	uint32_t bg_color;
} conf_t;

static int32_t read_config(conf_t* conf, const char* fname) {
	sconf_t *sconf = sconf_load(fname);	
	if(sconf == NULL)
		return -1;

	const char* v = sconf_get(sconf, "bg_color");
	if(v[0] != 0) 
		conf->bg_color = argb_int(atoi_base(v, 16));
	v = sconf_get(sconf, "fg_color");
	if(v[0] != 0) 
		conf->fg_color = argb_int(atoi_base(v, 16));
	v = sconf_get(sconf, "font");
	if(v[0] != 0) 
		conf->font = get_font_by_name(v);
	sconf_free(sconf);
	return 0;
}

static int run(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	conf_t conf;
	read_config(&conf, "/etc/x/gconsole.conf");

	int fd = open("/dev/keyb0", O_RDONLY);
	if(fd < 0)
		return -1;

	x_t* xp = x_open(10, 100, 800, 600, "gconsole");
	if(xp == NULL) {
		close(fd);
		return -1;
	}
	graph_t* g = x_graph(xp);

	console_t console;
	console_init(&console);
	console.g = g;

	console.font = conf.font;
	console.fg_color = conf.fg_color;
	console.bg_color = conf.bg_color;
	console_reset(&console);

	int krd = 0;
	xevent_t xev;
	while(1) {
		if(krd != 1) {
			if(x_get_event(xp, &xev) == 0) {
				if(xev.type == XEVT_KEYB)
					krd = 1;
			}
		}
		else {
			char c = xev.value.keyboard.value;
			if(write_nblock(1, &c, 1) == 1)
				krd = 0;
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
		x_update(xp);
	}

	close(fd);
	console_close(&console);
	x_close(xp);
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
	return exec("/bin/shell");
}
