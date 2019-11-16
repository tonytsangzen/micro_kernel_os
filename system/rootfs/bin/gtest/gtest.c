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
	while(i<300) {
		snprintf(str, 31, "paint = %d", i++);
		clear(g, 0xff0000ff);
		draw_text(g, 30, 10, str, font, 0xffffffff);
		x_update(x);
		sleep(0);
	}
	x_close(x);
	return 0;
} 
