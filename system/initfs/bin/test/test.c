#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <shm.h>
#include <sys/wait.h>
#include <graph/graph.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int fd = open("/dev/fb0", 0);
	if(fd < 0)
		return -1;

	int id, size;
	id = dma(fd, &size);
	printf("%d, %d\n", id, size);
	void* p = shm_map(id);

	graph_t* g = graph_new(p, 1024, 768);
	font_t* font = get_font_by_name("16x32");

	char s[32];
	int i = 0;
	while(i<1000) {
		snprintf(s, 31, "hello,dma %d", i++);
		clear(g, 0xffffffff);
		draw_text(g, 100, 400, s, font, 0xff0000ff);
		flush(fd);
		sleep(0);
	}

	shm_unmap(id);
	close(fd);
	return 0;
}
