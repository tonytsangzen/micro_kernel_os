#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <vfs.h>
#include <vdevice.h>
#include <syscall.h>
#include <dev/device.h>
#include <shm.h>
#include <graph/graph.h>
#include <dev/fbinfo.h>
#include <ipc.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xwm.h>
#include <global.h>
#include <thread.h>
#include <proclock.h>

#define X_EVENT_MAX 16

typedef struct st_xview_ev {
	xevent_t event;
	struct st_xview_ev* next;
} xview_event_t;

typedef struct st_xview {
	int fd;
	int from_pid;
	graph_t* g;
	xinfo_t xinfo;
	int dirty;

	struct st_xview *next;
	struct st_xview *prev;
	xview_event_t* event_head;
	xview_event_t* event_tail;
	int event_num;
} xview_t;

typedef struct {
	gpos_t old_pos;
	gpos_t cpos;
	gpos_t offset;
	gsize_t size;
	graph_t* g;
} cursor_t;

typedef struct {
	xview_t* view; //moving or resizing;
	gpos_t old_pos;
} x_current_t;

typedef struct {
	int actived;
	int fb_fd;
	int keyb_fd;
	int mouse_fd;
	int xwm_pid;
	int shm_id;
	int dirty;
	int need_repaint;
	graph_t* g;
	cursor_t cursor;

	xview_t* view_head;
	xview_t* view_tail;

	x_current_t current;
	proc_lock_t lock;
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

static void draw_win_frame(x_t* x, xview_t* view) {
	if((view->xinfo.style & X_STYLE_NO_FRAME) != 0)
		return;

	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, XWM_CNTL_DRAW_FRAME); // 0 for draw view frame
	proto_add_int(&in, x->shm_id);
	proto_add_int(&in, x->g->w);
	proto_add_int(&in, x->g->h);
	proto_add(&in, &view->xinfo, sizeof(xinfo_t));
	if(view == x->view_tail)
		proto_add_int(&in, 1); //top win
	else
		proto_add_int(&in, 0);

	ipc_call(x->xwm_pid, &in, &out);
	proto_clear(&in);
	proto_clear(&out);
}

static void draw_desktop(x_t* x) {
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, XWM_CNTL_DRAW_DESKTOP); // 1 for draw desktop
	proto_add_int(&in, x->shm_id);
	proto_add_int(&in, x->g->w);
	proto_add_int(&in, x->g->h);

	ipc_call(x->xwm_pid, &in, &out);
	proto_clear(&in);
	proto_clear(&out);
}

static void draw_mask(graph_t* g, int x, int y, int w, int h) {
	if(x < 0) {
		w += x;
		x = 0;
	}
	if(y < 0) {
		h += y;
		y = 0;
	}
	if((x+w) >= (int)g->w)
		w = g->w - x - 1;
	if((y+h) >= (int)g->h) 
		h = g->h - y - 1;

	int i, j;
	int step = 4;
	for(j=0; j<h; j+=step) {
		for(i=0; i<w; i+=step) {
			pixel(g, i+x, j+y, 0xff000000);
			pixel(g, i+x+1, j+y+1, 0xffffffff);
		}
	}
}

static int draw_view(x_t* xp, xview_t* view) {
	if(xp->dirty == 0 && view->dirty == 0)
		return 0;

	if(view->g != NULL) {
		if(xp->current.view != view) { //drag and moving
			if((view->xinfo.style & X_STYLE_ALPHA) != 0) {
				blt_alpha(view->g, 0, 0, 
						view->xinfo.r.w,
						view->xinfo.r.h,
						xp->g,
						view->xinfo.r.x,
						view->xinfo.r.y,
						view->xinfo.r.w,
						view->xinfo.r.h, 0xff);
			}
			else {
				blt(view->g, 0, 0, 
						view->xinfo.r.w,
						view->xinfo.r.h,
						xp->g,
						view->xinfo.r.x,
						view->xinfo.r.y,
						view->xinfo.r.w,
						view->xinfo.r.h);
			}
		}
		else {
			draw_mask(xp->g, 
					view->xinfo.r.x,
					view->xinfo.r.y,
					view->xinfo.r.w,
					view->xinfo.r.h);
		}
	}

	draw_win_frame(xp, view);
	view->dirty = 0;
	return 0;
}

static inline void x_dirty(x_t* x) {
	x->dirty = 1;
	x->need_repaint = 1;
}

static void remove_view(x_t* x, xview_t* view) {
	if(view->prev != NULL)
		view->prev->next = view->next;
	if(view->next != NULL)
		view->next->prev = view->prev;
	if(x->view_tail == view)
		x->view_tail = view->prev;
	if(x->view_head == view)
		x->view_head = view->next;
	view->next = view->prev = NULL;
	x_dirty(x);
}

static void x_push_event(xview_t* view, xview_event_t* e, uint8_t must) {
	if(view->event_num >= X_EVENT_MAX && must == 0)
		return;

	if(view->event_tail != NULL)
		view->event_tail->next = e;
	else
		view->event_head = e;
	view->event_tail = e;
	view->event_num++;
}

static void push_view(x_t* x, xview_t* view) {
	if((view->xinfo.style & X_STYLE_NO_FOCUS) == 0) {
		if(x->view_tail != NULL) {
			xview_event_t* e = (xview_event_t*)malloc(sizeof(xview_event_t));
			e->next = NULL;
			e->event.type = XEVT_WIN;
			e->event.value.window.event = XEVT_WIN_UNFOCUS;
			x_push_event(x->view_tail, e, 1);

			x->view_tail->next = view;
			view->prev = x->view_tail;
			x->view_tail = view;
		}
		else {
			x->view_tail = x->view_head = view;
		}
		xview_event_t* e = (xview_event_t*)malloc(sizeof(xview_event_t));
		e->next = NULL;
		e->event.type = XEVT_WIN;
		e->event.value.window.event = XEVT_WIN_FOCUS;
		x_push_event(view, e, 1);
	}
	else {
		if(x->view_head != NULL) {
			x->view_head->prev = view;
			view->next = x->view_head;
			x->view_head = view;
		}
		else {
			x->view_tail = x->view_head = view;
		}
	}
	x_dirty(x);
}

static void x_del_view(x_t* x, xview_t* view) {
	remove_view(x, view);
	free(view);

	if(x->view_tail != NULL) {
		xview_event_t* e = (xview_event_t*)malloc(sizeof(xview_event_t));
		e->next = NULL;
		e->event.type = XEVT_WIN;
		e->event.value.window.event = XEVT_WIN_FOCUS;
		x_push_event(x->view_tail, e, 1);
	}
}

static void hide_cursor(x_t* x) {
	if(x->cursor.g == NULL) {
		x->cursor.g = graph_new(NULL, x->cursor.size.w, x->cursor.size.h);
		blt(x->g,
				x->cursor.old_pos.x - x->cursor.offset.x,
				x->cursor.old_pos.y - x->cursor.offset.y,
				x->cursor.size.w,
				x->cursor.size.h,
				x->cursor.g,
				0,
				0, 
				x->cursor.size.w,
				x->cursor.size.h);
	}
	else  {
		blt(x->cursor.g,
				0,
				0,
				x->cursor.size.w,
				x->cursor.size.h,
				x->g,
				x->cursor.old_pos.x - x->cursor.offset.x,
				x->cursor.old_pos.y - x->cursor.offset.x,
				x->cursor.size.w,
				x->cursor.size.h);
	}
}

static inline void draw_cursor(x_t* x) {
	int32_t mx = x->cursor.cpos.x - x->cursor.offset.x;
	int32_t my = x->cursor.cpos.y - x->cursor.offset.y;
	int32_t mw = x->cursor.size.w;
	int32_t mh = x->cursor.size.h;

	if(x->cursor.g == NULL)
		return;

	blt(x->g, mx, my, mw, mh,
			x->cursor.g, 0, 0, mw, mh);

	line(x->g, mx+1, my, mx+mw-1, my+mh-2, 0xffffffff);
	line(x->g, mx, my, mx+mw-1, my+mh-1, 0xff000000);
	line(x->g, mx, my+1, mx+mw-2, my+mh-1, 0xffffffff);

	line(x->g, mx, my+mh-2, mx+mw-2, my, 0xffffffff);
	line(x->g, mx, my+mh-1, mx+mw-1, my, 0xff000000);
	line(x->g, mx+1, my+mh-1, mx+mw-1, my+1, 0xffffffff);
	x->cursor.old_pos.x = x->cursor.cpos.x;
	x->cursor.old_pos.y = x->cursor.cpos.y;
}

static void x_repaint(x_t* x) {
	if(x->actived == 0 ||
			(x->need_repaint == 0 && x->dirty == 0))
		return;
	x->need_repaint = 0;

	hide_cursor(x);
	if(x->dirty != 0)
		draw_desktop(x);

	xview_t* view = x->view_head;
	while(view != NULL) {
		draw_view(x, view);
		view = view->next;
	}
	draw_cursor(x);
	flush(x->fb_fd);
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

static int x_update(int fd, int from_pid, x_t* x) {
	if(fd < 0)
		return -1;
	
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL)
		return -1;

	view->dirty = 1;
	if(view != x->view_tail ||
			(view->xinfo.style & X_STYLE_ALPHA) != 0) {
		x_dirty(x);
	}
	x->need_repaint = 1;
	return 0;
}

static int x_update_info(int fd, int from_pid, proto_t* in, x_t* x) {
	xinfo_t xinfo;
	int sz = sizeof(xinfo_t);
	if(fd < 0 || proto_read_to(in, &xinfo, sz) != sz)
		return -1;
	
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL)
		return -1;
	
	int shm_id = view->xinfo.shm_id;
	if(shm_id == 0 ||
			view->g == NULL ||
			view->xinfo.r.w != xinfo.r.w ||
			view->xinfo.r.h != xinfo.r.h) {
		if(view->g != NULL && shm_id > 0) {
			graph_free(view->g);
			shm_unmap(shm_id);
		}
		shm_id = shm_alloc(xinfo.r.w * xinfo.r.h * 4, 1);
		void* p = shm_map(shm_id);
		if(p == NULL) 
			return -1;
		view->g = graph_new(p, xinfo.r.w, xinfo.r.h);
	}
	view->dirty = 1;
	x->need_repaint = 1;

	if(view != x->view_tail ||
			view->xinfo.r.x != xinfo.r.x ||
			view->xinfo.r.y != xinfo.r.y ||
			view->xinfo.r.w != xinfo.r.w ||
			view->xinfo.r.h != xinfo.r.h ||
			(view->xinfo.style & X_STYLE_ALPHA) != 0) {
		x_dirty(x);
	}
	memcpy(&view->xinfo, &xinfo, sizeof(xinfo_t));
	view->xinfo.shm_id = shm_id;

	return 0;
}

static int x_get_event(int fd, int from_pid, x_t* x, proto_t* out) {
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL || view->event_head == NULL)
		return -1;

	xview_event_t* e = view->event_head;
	view->event_head = view->event_head->next;
	if(view->event_head == NULL)
		view->event_tail = NULL;

	proto_add(out, &e->event, sizeof(xevent_t));
	free(e);
	if(view->event_num > 0)
		view->event_num--;
	return 0;
}

static int x_get_info(int fd, int from_pid, x_t* x, proto_t* out) {
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL)
		return -1;
	proto_add(out, &view->xinfo, sizeof(xinfo_t));
	return 0;
}

static int x_is_top(int fd, int from_pid, x_t* x, proto_t* out) {
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL || x->view_tail == NULL)
		return -1;

	if(view == x->view_tail)
		proto_add_int(out, 0);
	else
		proto_add_int(out, -1);
	return 0;
}

static int x_scr_info(x_t* x, proto_t* out) {
	xscreen_t scr;	
	scr.id = 0;
	scr.size.w = x->g->w;
	scr.size.h = x->g->h;
	proto_add(out, &scr, sizeof(xscreen_t));
	return 0;
}

static int get_xwm_workspace(x_t* x, int style, grect_t* rin, grect_t* rout) {
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, XWM_CNTL_GET_WORKSPACE); 
	proto_add_int(&in, style);
	proto_add(&in, rin, sizeof(grect_t));

	ipc_call(x->xwm_pid, &in, &out);

	proto_clear(&in);
	proto_read_to(&out, rout, sizeof(grect_t));
	proto_clear(&out);
	return 0;
}

static int x_workspace(x_t* x, proto_t* in, proto_t* out) {
	grect_t r;
	int style = proto_read_int(in);
	proto_read_to(in, &r, sizeof(grect_t));
	get_xwm_workspace(x, style, &r, &r);
	proto_add(out, &r, sizeof(grect_t));
	return 0;
}

static int xserver_fcntl(int fd, int from_pid, fsinfo_t* info, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)info;
	x_t* x = (x_t*)p;

	if(cmd == X_CNTL_UPDATE) {
		return x_update(fd, from_pid, x);
	}	
	else if(cmd == X_CNTL_UPDATE_INFO) {
		return x_update_info(fd, from_pid, in, x);
	}
	else if(cmd == X_CNTL_GET_INFO) {
		return x_get_info(fd, from_pid, x, out);
	}
	else if(cmd == X_CNTL_GET_EVT) {
		return x_get_event(fd, from_pid, x, out);
	}
	else if(cmd == X_CNTL_SCR_INFO) {
		return x_scr_info(x, out);
	}
	else if(cmd == X_CNTL_WORKSPACE) {
		return x_workspace(x, in, out);
	}
	else if(cmd == X_CNTL_IS_TOP) {
		return x_is_top(fd, from_pid, x, out);
	}

	return 0;
}

static int xserver_open(int fd, int from_pid, fsinfo_t* info, int oflag, void* p) {
	(void)oflag;
	(void)info;
	x_t* x = (x_t*)p;
	if(fd < 0)
		return -1;

	xview_t* view = (xview_t*)malloc(sizeof(xview_t));
	if(view == NULL)
		return -1;
	memset(view, 0, sizeof(xview_t));
	view->fd = fd;
	view->from_pid = from_pid;
	push_view(x, view);
	return 0;
}

static int xserver_closed(int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)info;
	x_t* x = (x_t*)p;
	
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL)
		return -1;

	x_del_view(x, view);	
	return 0;
}

static int x_init(x_t* x) {
	memset(x, 0, sizeof(x_t));
	x->view_head = NULL;	
	x->view_tail = NULL;	

	int fd = open("/dev/keyb0", O_RDONLY);
	if(fd < 0)
		return -1;
	x->keyb_fd = fd;

	fd = open("/dev/mouse0", O_RDONLY);
	if(fd < 0) {
		close(x->keyb_fd);
		return -1;
	}
	x->mouse_fd = fd;

	fd = open("/dev/fb0", O_RDONLY);
	if(fd < 0) {
		close(x->keyb_fd);
		close(x->mouse_fd);
		return -1;
	}
	x->fb_fd = fd;

	int id = dma(fd, NULL);
	if(id <= 0) {
		close(x->keyb_fd);
		close(x->mouse_fd);
		close(x->fb_fd);
		return -1;
	}

	void* gbuf = shm_map(id);
	if(gbuf == NULL) {
		close(x->keyb_fd);
		close(x->mouse_fd);
		close(x->fb_fd);
		return -1;
	}

	fbinfo_t info;
	proto_t out;
	proto_init(&out, NULL, 0);

	if(fcntl_raw(fd, CNTL_INFO, NULL, &out) != 0) {
		shm_unmap(id);
		close(x->keyb_fd);
		close(x->mouse_fd);
		close(x->fb_fd);
		return -1;
	}

	proto_read_to(&out, &info, sizeof(fbinfo_t));
	x->g = graph_new(gbuf, info.width, info.height);
	proto_clear(&out);
	x->shm_id = id;
	x_dirty(x);

	x->cursor.size.w = 15;
	x->cursor.size.h = 15;
	x->cursor.offset.x = 8;
	x->cursor.offset.y = 8;
	x->cursor.cpos.x = info.width/2;
	x->cursor.cpos.y = info.height/2; 

	x->lock = proc_lock_new();
	return 0;
}	

static int get_win_frame_pos(x_t* x, xview_t* view) {
	if((view->xinfo.style & X_STYLE_NO_FRAME) != 0)
		return -1;

	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, XWM_CNTL_GET_POS); // 2 for get_win_frame_posos
	proto_add_int(&in, x->cursor.cpos.x);
	proto_add_int(&in, x->cursor.cpos.y);
	proto_add(&in, &view->xinfo, sizeof(xinfo_t));
	ipc_call(x->xwm_pid, &in, &out);
	proto_clear(&in);

	int res = proto_read_int(&out);
	proto_clear(&out);
	return res;
}

static xview_t* get_mouse_owner(x_t* x, int* win_frame_pos) {
	xview_t* view = x->view_tail;
	if(win_frame_pos != NULL)
		*win_frame_pos = -1;

	while(view != NULL) {
		if(x->cursor.cpos.x >= view->xinfo.r.x && x->cursor.cpos.x < (view->xinfo.r.x+view->xinfo.r.w) &&
				x->cursor.cpos.y >= view->xinfo.r.y && x->cursor.cpos.y < (view->xinfo.r.y+view->xinfo.r.h))
			return view;
		int pos = get_win_frame_pos(x, view);
		if(pos >= 0) {
			if(win_frame_pos != NULL)
				*win_frame_pos = pos;
			return view;
		}
		view = view->prev;
	}
	return NULL;
}

static void mouse_cxy(x_t* x, int32_t rx, int32_t ry) {
	x->cursor.cpos.x += rx;
	x->cursor.cpos.y += ry;
	if(x->cursor.cpos.x < 0)
		x->cursor.cpos.x = 0;
	if(x->cursor.cpos.x >= (int32_t)x->g->w)
		x->cursor.cpos.x = x->g->w;
	if(x->cursor.cpos.y < 0)
		x->cursor.cpos.y = 0;
	if(x->cursor.cpos.y >= (int32_t)x->g->h)
		x->cursor.cpos.y = x->g->h;
}

static int mouse_handle(x_t* x, int8_t state, int32_t rx, int32_t ry) {
	mouse_cxy(x, rx, ry);

	xview_event_t* e = (xview_event_t*)malloc(sizeof(xview_event_t));
	e->next = NULL;
	e->event.type = XEVT_MOUSE;
	e->event.state = XEVT_MOUSE_MOVE;
	e->event.value.mouse.x = x->cursor.cpos.x;
	e->event.value.mouse.y = x->cursor.cpos.y;
	e->event.value.mouse.rx = rx;
	e->event.value.mouse.ry = ry;

	int pos = -1;
	xview_t* view = NULL;
	if(x->current.view != NULL)
		view = x->current.view;
	else
		view = get_mouse_owner(x, &pos);

	if(view == NULL) {
		free(e);
		return -1;
	}

	if(state == 2) { //down
		if(view != x->view_tail) {
			remove_view(x, view);
			push_view(x, view);
		}

		if(pos == XWM_FRAME_CLOSE) { //window close
			e->event.type = XEVT_WIN;
			e->event.value.window.event = XEVT_WIN_CLOSE;
		}
		else if(pos == XWM_FRAME_MAX) {
			e->event.type = XEVT_WIN;
			e->event.value.window.event = XEVT_WIN_MAX;
		}
		else { // mouse down
			if(pos == XWM_FRAME_TITLE) {//window title 
				x->current.view = view;
				x->current.old_pos.x = x->cursor.cpos.x;
				x->current.old_pos.y = x->cursor.cpos.y;
				x_dirty(x);
			}
			e->event.state = XEVT_MOUSE_DOWN;
		}
	}
	else if(state == 1) {
		e->event.state = XEVT_MOUSE_UP;
		if(x->current.view == view) {
			x_dirty(x);
		}
		x->current.view = NULL;
	}

	if(x->current.view == view) {
		int mrx = x->cursor.cpos.x - x->current.old_pos.x;
		int mry = x->cursor.cpos.y - x->current.old_pos.y;
		if(abs32(mrx) > 16 || abs32(mry) > 16) {
			x->current.old_pos.x = x->cursor.cpos.x;
			x->current.old_pos.y = x->cursor.cpos.y;
			view->xinfo.r.x += mrx;
			view->xinfo.r.y += mry;
			x_dirty(x);
		}
	}
	if(e->event.type == XEVT_MOUSE && e->event.state == XEVT_MOUSE_MOVE)
		x_push_event(view, e, 0);
	else
		x_push_event(view, e, 1);
	return -1;
}

static void read_thread(void* p) {
	x_t* x = (x_t*)p;
	while(1) {
		if(x->actived == 0)  {
			usleep(10000);
			continue;
		}

		int8_t v;
		//read keyb
		int rd = read_nblock(x->keyb_fd, &v, 1);
		if(rd == 1) {
			xview_t* topv = x->view_tail; 
			if(topv != NULL) {
				xview_event_t* e = (xview_event_t*)malloc(sizeof(xview_event_t));
				e->next = NULL;
				e->event.type = XEVT_KEYB;
				e->event.value.keyboard.value = v;

				proc_lock(x->lock);
				x_push_event(topv, e, 1);
				proc_unlock(x->lock);
			}
		}

		//read mouse
		int8_t mv[4];
		if(read_nblock(x->mouse_fd, mv, 4) == 4) {
			proc_lock(x->lock);
			mouse_handle(x, mv[0], mv[1], mv[2]);
			x->need_repaint = 1;
			proc_unlock(x->lock);
		}
		sleep(0);
	}
}

static int xserver_loop_step(void* p) {
	x_t* x = (x_t*)p;
	const char* cc = get_global("current_console");
	if(cc[0] == 'x') {
		if(x->actived == 0) {
			x_dirty(x);
		}
		x->actived = 1;
	}
	else
		x->actived = 0;
	if(x->actived == 0)  {
		usleep(10000);
		return -1;
	}

	proc_lock(x->lock);
	x_repaint(x);	
	proc_unlock(x->lock);
	return 0;
}

static void x_close(x_t* x) {
	proc_lock_free(x->lock);
	close(x->keyb_fd);
	close(x->mouse_fd);
	graph_free(x->g);
	shm_unmap(x->shm_id);
	close(x->fb_fd);
}

int main(int argc, char** argv) {
	fsinfo_t mnt_point;
	const char* mnt_name = argc > 1 ? argv[1]: "/dev/x";
	vfs_create(mnt_name, &mnt_point, FS_TYPE_DEV);

	int pid = fork();
	if(pid == 0) {
		exec("/sbin/x/xwm");
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xserver");
	dev.mount = xserver_mount;
	dev.fcntl = xserver_fcntl;
	//dev.close = xserver_close;
	dev.closed = xserver_closed;
	dev.open = xserver_open;
	dev.loop_step= xserver_loop_step;
	dev.umount = xserver_umount;

	mount_info_t mnt_info;
	strcpy(mnt_info.dev_name, dev.name);
	mnt_info.dev_index = 0;
	mnt_info.access = 0;

	x_t x;
	if(x_init(&x) == 0) {
		x.xwm_pid = pid;
		thread_create(read_thread, &x);
		device_run(&dev, &mnt_point, &mnt_info, &x, 0);
		x_close(&x);
	}
	return 0;
}
