#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <vprintf.h>
#include <shm.h>
#include <dev/fbinfo.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int fd_fb = open("/dev/fb0", 0);
	if(fd_fb < 0)
		return -1;

	int id, size;
	id = dma(fd_fb, &size);
	void* p = shm_map(id);

	fbinfo_t info;
	proto_t* out = proto_new(NULL, 0);
	if(cntl_raw(fd_fb, CNTL_INFO, NULL, out) == 0)
		proto_read_to(out, &info, sizeof(fbinfo_t));
	proto_free(out);

	graph_t* g = graph_new(p, info.width, info.height);
	font_t* font = get_font_by_name("16x32");

	int fd = open("/dev/mouse0", O_RDONLY);
	char str[32] = "Hello, EwokOS";
	while(1) {
		clear(g, 0xff0000ff);
		draw_text(g, 10, 10, str, font, 0xffffffff);
		flush(fd_fb);

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

	close(fd_fb);
	shm_unmap(id);

	return 0;
} 
