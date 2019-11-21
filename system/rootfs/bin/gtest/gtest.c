#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <vprintf.h>
#include <x/xclient.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	x_t* x = x_open(100+(getpid()-6)*40, 100+(getpid()-6)*40, 600, 200, "gtest");
	graph_t* g = x_graph(x);
	font_t* font = get_font_by_name("16x32");

	char str[32];
	int i=0;

	xevent_t xev;
	while(1) {
		if(x_get_event(x, &xev) == 0) {
			if(xev.type == XEVT_KEYB)
				break;
		}
		snprintf(str, 31, "paint = %d", i++);
		clear(g, 0xff0000ff);
		draw_text(g, 30, 10, str, font, 0xffffffff);
		draw_text(g, 30, g->h-20, "press anykey to quit......", get_font_by_name("8x16"), 0xffffffff);
		x_update(x);

		if(i > 100) {
			x->xinfo.r.x = 200;
			x->xinfo.r.y = 200;
			strcpy(x->xinfo.title, "hahaha");
			x_update(x);
		}
		sleep(0);
	}

	x_close(x);
	return 0;
} 
