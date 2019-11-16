#include <x/xclient.h>
#include <shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

int x_update(x_t* x) {
	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add(&in, &x->xinfo, sizeof(xinfo_t));

	int ret = cntl_raw(x->fd, X_CNTL_UPDATE, &in, NULL);
	proto_clear(&in);
	return ret;
}

static int x_new(x_t* x) {
	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add(&in, &x->xinfo, sizeof(xinfo_t));

	int ret = cntl_raw(x->fd, X_CNTL_NEW, &in, NULL);
	proto_clear(&in);
	return ret;
}

x_t* x_open(int x, int y, int w, int h, const char* title) {
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
	strncpy(ret->xinfo.title, title, X_TITLE_MAX-1);
	ret->g = graph_new(gbuf, w, h);

	x_new(ret);
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

int x_get_event(x_t* x, xevent_t* ev) {
	proto_t out;
	proto_init(&out, NULL, 0);

	int ret = cntl_raw(x->fd, X_CNTL_GET_EVT, NULL, &out);
	if(ret == 0)
		proto_read_to(&out, ev, sizeof(xevent_t));
	proto_clear(&out);
	return ret;
}
