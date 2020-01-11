#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <vprintf.h>
#include <x/xclient.h>

void on_focus(x_t* x, void* p) {
	(void)x;
	*(int*)p = 1;
}

void on_unfocus(x_t* x, void* p) {
	(void)x;
	*(int*)p = 0;
}

#define KEY_V_UP        0x1
#define KEY_V_DOWN      0x2
#define KEY_V_LEFT      0x4
#define KEY_V_RIGHT     0x8
#define KEY_V_PRESS     0x10
#define KEY_V_1         0x20
#define KEY_V_2         0x40
#define KEY_V_3         0x80

const char* get_key(int key) {
	switch(key) {
	case KEY_V_UP:
		return "up";
	case KEY_V_DOWN:
		return "down";
	case KEY_V_LEFT:
		return "left";
	case KEY_V_RIGHT:
		return "right";
	case KEY_V_PRESS:
		return "pressed";
	case KEY_V_1:
		return "key1";
	case KEY_V_2:
		return "key2";
	case KEY_V_3:
		return "key3";
	}
	return "";
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	xscreen_t scr;
	x_screen_info(&scr);
	x_t* x = x_open(10, 10, 220, 200, "joystick", X_STYLE_NORMAL);
	int top = 0;
	x->data = &top;
	x->on_focus = on_focus;
	x->on_unfocus = on_unfocus;

	font_t* font = font_by_name("16x32");

	int fd = open("/dev/joystick", O_RDONLY);
	if(fd < 0) {
		x_close(x);
		return -1;
	}

	while(x->closed == 0) {
		xevent_t xev;
		if(x_get_event(x, &xev) == 0) {
			//if(xev.type == XEVT_KEYB)
			//	break;
		}
		if(top == 1) {
			char key = 0;
			read(fd, &key, 1);
			graph_t* g = x_get_graph(x);
			clear(g, argb_int(0xff0000ff));
			draw_text(g, 10, 10, "key:", font, 0xffffffff);
			draw_text(g, 10+32, 10+32, get_key(key), font, argb_int(0xffff8800));
			draw_text(g, 10, g->h-20, "joystick demo.", font_by_name("8x16"), 0xffffffff);
			x_release_graph(x, g);
			x_update(x);
		}
	}

	close(fd);
	x_close(x);
	return 0;
} 
