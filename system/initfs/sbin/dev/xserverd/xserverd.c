#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <vfs.h>
#include <vdevice.h>
#include <svc_call.h>
#include <dev/device.h>
#include <shm.h>
#include <graph/graph.h>
#include <dev/fbinfo.h>
#include <x/xcntl.h>

typedef struct st_xview {
	int32_t fd;
	int32_t from_pid;
	xinfo_t xinfo;
	int32_t dirty;

	struct st_xview *next;
	struct st_xview *prev;
} xview_t;

typedef struct {
	int gfd;
	int shm_id;
	int32_t dirty;
	graph_t* g;

	xview_t* view_head;
	xview_t* view_tail;
} x_t;

static int xserver_mount(fsinfo_t* mnt_point, mount_info_t* mnt_info, void* p) {
	(void)p;

	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	strcpy(info.name, mnt_point->name);
	info.type = FS_TYPE_DEV;
	vfs_new_node(&info);

	if(vfs_mount(mnt_point, &info, mnt_info) != 0) {
		vfs_del(&info);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

static int xserver_umount(fsinfo_t* info, void* p) {
	(void)p;
	vfs_umount(info);
	return 0;
}

static int draw_view(x_t* x, xview_t* view) {
	void* gbuf = shm_map(view->xinfo.shm_id);
	if(gbuf == NULL) {
		return -1;
	}

	if(x->dirty == 0 && view->dirty == 0) {
		shm_unmap(view->xinfo.shm_id);
		return 0;
	}

	graph_t* g = graph_new(gbuf, view->xinfo.r.w, view->xinfo.r.h);
	blt(g, 0, 0, view->xinfo.r.w, view->xinfo.r.h,
			x->g, view->xinfo.r.x, view->xinfo.r.y, view->xinfo.r.w, view->xinfo.r.h);
	graph_free(g);
	shm_unmap(view->xinfo.shm_id);
	view->dirty = 0;
	return 0;
}

static void draw_desktop(x_t* xp) {
	clear(xp->g, 0xff000000);
	//background pattern
	int32_t x, y;
	for(y=10; y<(int32_t)xp->g->h; y+=10) {
		for(x=0; x<(int32_t)xp->g->w; x+=10) {
			pixel(xp->g, x, y, 0xff444444);
		}
	}
}

static void x_del_view(x_t* x, xview_t* view) {
	if(view->prev != NULL)
		view->prev->next = view->next;
	if(view->next != NULL)
		view->next->prev = view->prev;
	if(x->view_tail == view)
		x->view_tail = view->prev;
	if(x->view_head == view)
		x->view_head = view->next;
	x->dirty = 1;
}

static void x_repaint(x_t* x) {
	if(x->dirty != 0)
		draw_desktop(x);

	xview_t* view = x->view_head;
	while(view != NULL) {
		int res = draw_view(x, view);
		xview_t* v = view;
		view = view->next;
		if(res != 0) //client close/broken. remove it.
			x_del_view(x, v);
	}

	flush(x->gfd);
	x->dirty = 0;
}

static xview_t* x_get_view(x_t* x, int fd, int from_pid) {
	xview_t* view = x->view_head;
	while(view != NULL) {
		if(view->fd == fd && view->from_pid == from_pid)
			return view;
		view = view->next;
	}
	return NULL;
}

static int x_update(int fd, int from_pid, proto_t* in, x_t* x) {
	xinfo_t xinfo;
	int sz = sizeof(xinfo_t);
	if(fd < 0 || proto_read_to(in, &xinfo, sz) != sz)
		return -1;
	
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL)
		return -1;
	memcpy(&view->xinfo, &xinfo, sizeof(xinfo_t));

	view->dirty = 1;
	if(view != x->view_tail)
		x->dirty = 1;

	x_repaint(x);
	return 0;
}

static int x_new_view(int fd, int from_pid, proto_t* in, x_t* x) {
	xinfo_t xinfo;
	int sz = sizeof(xinfo_t);
	if(fd < 0 || proto_read_to(in, &xinfo, sz) != sz)
		return -1;
	xview_t* view = (xview_t*)malloc(sizeof(xview_t));
	if(view == NULL)
		return -1;
	memset(view, 0, sizeof(xview_t));

	memcpy(&view->xinfo, &xinfo, sizeof(xinfo_t));
	view->dirty = 1;
	view->fd = fd;
	view->from_pid = from_pid;
	if(x->view_tail != NULL) {
		x->view_tail->next = view;
		view->prev = x->view_tail;
		x->view_tail = view;
	}
	else {
		x->view_tail = x->view_head = view;
	}
	x_repaint(x);
	return 0;
}

static int xserver_close(int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)info;
	x_t* x = (x_t*)p;
	
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL)
		return -1;

	x_del_view(x, view);	
	return 0;
}

static int xserver_cntl(int fd, int from_pid, fsinfo_t* info, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)info;
	(void)out;
	x_t* x = (x_t*)p;

	if(cmd == X_CNTL_NEW) {
		return x_new_view(fd, from_pid, in, x);
	}
	else if(cmd == X_CNTL_UPDATE) {
		return x_update(fd, from_pid, in, x);
	}
	return 0;
}

static int x_init(x_t* x) {
	x->view_head = NULL;	
	x->view_tail = NULL;	

	int fd = open("/dev/fb0", 0);
	if(fd < 0)
		return -1;

	int id = dma(fd, NULL);
	void* gbuf = shm_map(id);

	fbinfo_t info;
	proto_t* out = proto_new(NULL, 0);
	if(cntl_raw(fd, CNTL_INFO, NULL, out) == 0)
		proto_read_to(out, &info, sizeof(fbinfo_t));
	proto_free(out);

	x->g = graph_new(gbuf, info.width, info.height);
	x->gfd = fd;
	x->shm_id = id;
	x->dirty = 1;
	return 0;
}	

static void x_close(x_t* x) {
	graph_free(x->g);
	shm_unmap(x->shm_id);
	close(x->gfd);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xserver");
	dev.mount = xserver_mount;
	dev.cntl = xserver_cntl;
	dev.close= xserver_close;
	dev.umount = xserver_umount;

	fsinfo_t dev_info;
	vfs_get("/dev", &dev_info);

	fsinfo_t mnt_point;
	memset(&mnt_point, 0, sizeof(fsinfo_t));
	strcpy(mnt_point.name, "xserver");
	mnt_point.type = FS_TYPE_DEV;

	vfs_new_node(&mnt_point);
	vfs_add(&dev_info, &mnt_point);

	mount_info_t mnt_info;
	strcpy(mnt_info.dev_name, dev.name);
	mnt_info.dev_index = 0;
	mnt_info.access = 0;

	x_t x;
	if(x_init(&x) == 0) {
		device_run(&dev, &mnt_point, &mnt_info, &x);
		x_close(&x);
	}
	return 0;
}
