#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <graph/graph.h>
#include <vprintf.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	graph_t* g = graph_from_fb();

	int fd = open("/dev/mouse0", O_RDONLY);

	char str[32] = "Hello, EwokOS";
	while(1) {
		clear(g, 0xff0000ff);
		draw_text(g, 10, 10, str, get_font_by_name("16x32"), 0xffffffff);
		graph_flush_fb(g);

		int8_t b, x, y, z;
		if(read(fd, &b, 1) == 1) {
			read(fd, &x, 1);
			read(fd, &y, 1);
			read(fd, &z, 1);

			snprintf(str, 31, "bt:%d,rx:%d,ry:%d,rz:%d", b, x, y, z);
		}
		sleep(0);
	}

	close(fd);
	graph_free(g);

	return 0;
} 
