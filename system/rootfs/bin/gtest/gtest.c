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

	xscreen_t scr;
	x_screen_info(&scr);
	x_t* x = x_open(100, 100, 600, 200, "gtest", X_STYLE_NORMAL);

	font_t* font = font_by_name("16x32");

	char str[32];
	int i=0;

	xevent_t xev;
	while(x->closed == 0) {
		if(x_get_event(x, &xev) == 0) {
			if(xev.type == XEVT_KEYB)
				break;
		}
		int res = x_is_top(x);
		if(res == 0) {
			snprintf(str, 31, "paint = %d", i++);
			graph_t* g = x_graph(x);
			clear(g, argb_int(0xff0000ff));
			draw_text(g, 30, 10, str, font, 0xffffffff);
			draw_text(g, 30, g->h-20, "press anykey to quit......", font_by_name("8x16"), 0xffffffff);
			x_update(x);
		}
		sleep(0);
	}

	x_close(x);
	return 0;
} 
