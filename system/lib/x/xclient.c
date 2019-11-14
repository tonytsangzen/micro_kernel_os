#include <x/xclient.h>
#include <shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

x_t* x_open(int x, int y, int w, int h) {
	if(w <= 0 || h <= 0)
		return NULL;

	int fd = open("/dev/xserver", O_RDWR);
	if(fd < 0)
		return NULL;
	int sz = w * h * 4;	
	int shm_id = shm_alloc(sz, SHM_PUBLIC);
	if(shm_id <= 0) {
		close(fd);
		return NULL;
	}

	void* gbuf = shm_map(shm_id);
	if(gbuf == NULL) {
		close(fd);
		return NULL;
	}	

	x_t* ret = (x_t*)malloc(sizeof(x_t));

	ret->fd = fd;
	ret->xinfo.shm_id = shm_id;
	ret->xinfo.r.x = x;
	ret->xinfo.r.y = y;
	ret->xinfo.r.w = w;
	ret->xinfo.r.h = h;
	ret->g = graph_new(gbuf, w, h);
	return ret;
}

graph_t* x_graph(x_t* x) {
	if(x == NULL)
		return NULL;
	return x->g;
}

void x_close(x_t* x) {
	if(x == NULL)
		return;

	shm_unmap(x->xinfo.shm_id);
	close(x->fd);
	graph_free(x->g);
	free(x);
}

