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

static int  x_get_workspace(int xfd, int style, grect_t* frame, grect_t* workspace) {
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, style);
	proto_add(&in, frame, sizeof(grect_t));
	int ret = cntl_raw(xfd, X_CNTL_WORKSPACE, &in, &out);
	proto_clear(&in);
	if(ret == 0) 
		proto_read_to(&out, workspace, sizeof(grect_t));
	proto_clear(&out);
	return ret;
}

x_t* x_open(int x, int y, int w, int h, const char* title, int style) {
	if(w <= 0 || h <= 0)
		return NULL;

	int fd = open("/dev/x", O_RDWR);
	if(fd < 0)
		return NULL;

	grect_t r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	x_get_workspace(fd, style, &r, &r);
		
	int sz = r.w * r.h * 4;	
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
	ret->closed = 0;
	ret->xinfo.shm_id = shm_id;
	ret->xinfo.style = style;
	memcpy(&ret->xinfo.r, &r, sizeof(grect_t));
	strncpy(ret->xinfo.title, title, X_TITLE_MAX-1);
	ret->g = graph_new(gbuf, r.w, r.h);

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

static void x_resize_to(x_t* xp, int x, int y, int w, int h) {
	if(x == xp->xinfo.r.x && y == xp->xinfo.r.y)
			if(w == xp->xinfo.r.w && h == xp->xinfo.r.h)
		return;
	
	shm_unmap(xp->xinfo.shm_id);
	graph_free(xp->g);

	int sz = w * h * 4;	
	int shm_id = shm_alloc(sz, SHM_PUBLIC);
	if(shm_id <= 0) {
		return;
	}
	void* gbuf = shm_map(shm_id);
	if(gbuf == NULL) {
		return;
	}	
	xp->xinfo.shm_id = shm_id;
	xp->xinfo.r.x = x;
	xp->xinfo.r.y = y;
	xp->xinfo.r.w = w;
	xp->xinfo.r.h = h;
	xp->g = graph_new(gbuf, w, h);
	x_update(xp);
}

static int win_event_handle(x_t* x, xevent_t* ev) {
	if(ev->value.window.event == XEVT_WIN_MOVE) {
		x->xinfo.r.x += ev->value.window.v0;
		x->xinfo.r.y += ev->value.window.v1;
		x_update(x);
	}
	else if(ev->value.window.event == XEVT_WIN_CLOSE) {
		x->closed = 1;
	}
	else if(ev->value.window.event == XEVT_WIN_MAX) {
		xscreen_t scr;
		if(x_screen_info(&scr) == 0) {
			grect_t r;
			r.x = 0;
			r.y = 0;
			r.w = scr.size.w;
			r.h = scr.size.h;
			x_get_workspace(x->fd, x->xinfo.style, &r, &r);
			x_resize_to(x, r.x, r.y, r.w, r.h);
			if(x->on_max != NULL)
				x->on_max(x, x->data);
		}
	}
	return 0;
}

int x_get_event(x_t* x, xevent_t* ev) {
	proto_t out;
	proto_init(&out, NULL, 0);

	int res = -1;
	if(cntl_raw(x->fd, X_CNTL_GET_EVT, NULL, &out) == 0) {
		proto_read_to(&out, ev, sizeof(xevent_t));
		if(ev->type == XEVT_WIN) 
			res = win_event_handle(x, ev);
		else
			res = 0;
	}
	proto_clear(&out);
	return res;
}

int x_screen_info(xscreen_t* scr) {
	int fd = open("/dev/x", O_RDWR);
	if(fd < 0)
		return -1;

	proto_t out;
	proto_init(&out, NULL, 0);
	int ret = cntl_raw(fd, X_CNTL_SCR_INFO, NULL, &out);
	close(fd);

	if(ret == 0)
		proto_read_to(&out, scr, sizeof(xscreen_t));
	proto_clear(&out);
	return ret;
}
