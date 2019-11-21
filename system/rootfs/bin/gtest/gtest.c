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

	x_t* x = x_open(100+(getpid()-6)*40, 100+(getpid()-6)*40, 600, 200, "gtest", X_STYLE_NO_TITLE);

	xscreen_t scr;
	x_screen_info(x, &scr);
	x->xinfo.r.x = (scr.size.w - x->xinfo.r.w) / 2;
	x->xinfo.r.y = (scr.size.h - x->xinfo.r.h) / 2;

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

		sleep(0);
	}

	x_close(x);
	return 0;
} 
