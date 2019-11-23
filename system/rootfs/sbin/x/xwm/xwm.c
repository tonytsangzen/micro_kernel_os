#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <ipc.h>
#include <shm.h>
#include <graph/graph.h>
#include <sconf.h>
#include <x/xcntl.h>
#include <x/xwm.h>

typedef struct {
	font_t* font;
	uint32_t desk_fg_color;
	uint32_t desk_bg_color;
	uint32_t fg_color;
	uint32_t bg_color;
	uint32_t top_bg_color;
	uint32_t top_fg_color;

	int32_t title_h;
} xwm_t;

static xwm_t _xwm;

static int32_t read_config(xwm_t* xwm, const char* fname) {
	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL)
		return -1;

	const char* v = sconf_get(conf, "desk_bg_color");
	if(v[0] != 0) 
		xwm->desk_bg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(conf, "desk_fg_color");
	if(v[0] != 0) 
		xwm->desk_fg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(conf, "bg_color");
	if(v[0] != 0) 
		xwm->bg_color = argb_int(atoi_base(v, 16));


	v = sconf_get(conf, "fg_color");
	if(v[0] != 0) 
		xwm->fg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(conf, "top_bg_color");
	if(v[0] != 0) 
		xwm->top_bg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(conf, "top_fg_color");
	if(v[0] != 0) 
		xwm->top_fg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(conf, "font");
	if(v[0] != 0) 
		xwm->font = get_font_by_name(v);

	sconf_free(conf);
	return 0;
}

static void draw_desktop(proto_t* in, proto_t* out) {
	int shm_id = proto_read_int(in);
	int xw = proto_read_int(in);
	int xh = proto_read_int(in);

	void* gbuf = shm_map(shm_id);
	if(gbuf != NULL) {
		graph_t* g = graph_new(gbuf, xw, xh);
		clear(g, _xwm.desk_bg_color);
		//background pattern
		int32_t x, y;
		for(y=10; y<(int32_t)g->h; y+=10) {
			for(x=0; x<(int32_t)g->w; x+=10) {
				pixel(g, x, y, _xwm.desk_fg_color);
			}
		}
		draw_text(g, 12, 12, "Ewok micro-kernel OS", _xwm.font, _xwm.bg_color);
		draw_text(g, 10, 10, "Ewok micro-kernel OS", _xwm.font, _xwm.fg_color);

		graph_free(g);
		shm_unmap(shm_id);
	}
	proto_add_int(out, 0);
}

static void draw_frame(proto_t* in, proto_t* out) {
	xinfo_t info;
	int shm_id = proto_read_int(in);
	int xw = proto_read_int(in);
	int xh = proto_read_int(in);
	proto_read_to(in, &info, sizeof(xinfo_t));
	int top = proto_read_int(in);

	void* gbuf = shm_map(shm_id);
	if(gbuf != NULL) {
		graph_t* g = graph_new(gbuf, xw, xh);

		uint32_t fg, bg;
		if(top == 0) {
			fg = _xwm.fg_color;
			bg = _xwm.bg_color;
		}
		else {
			fg = _xwm.top_fg_color;
			bg = _xwm.top_bg_color;
		}

		box(g, info.r.x, info.r.y, info.r.w, info.r.h, fg);//win box
		if((info.style & X_STYLE_NO_TITLE) == 0) {
			fill(g, info.r.x, info.r.y-_xwm.title_h, info.r.w, _xwm.title_h, bg);//title box
			box(g, info.r.x, info.r.y-_xwm.title_h, info.r.w, _xwm.title_h, fg);//title box
			box(g, info.r.x+info.r.w-_xwm.title_h, info.r.y-_xwm.title_h, _xwm.title_h, _xwm.title_h, fg);//close box
			draw_text(g, info.r.x+10, info.r.y-_xwm.title_h+2, info.title, _xwm.font, fg);//title
		}
		graph_free(g);
		shm_unmap(shm_id);
	}
	proto_add_int(out, 0);
}

static void get_pos(proto_t* in, proto_t* out) {
	xinfo_t info;
	int x = proto_read_int(in);
	int y = proto_read_int(in);
	proto_read_to(in, &info, sizeof(xinfo_t));

	int res = -1;
	if((info.style & X_STYLE_NO_TITLE) == 0) {
		if(x >= info.r.x && y >= info.r.y-_xwm.title_h &&
				x <= info.r.x+info.r.w-_xwm.title_h && y <= info.r.y)
			res = XWM_FRAME_TITLE;
		else if(x >= info.r.x+info.r.w-_xwm.title_h && y >= info.r.y-_xwm.title_h &&
				x <= info.r.x+info.r.w && y <= info.r.y)
			res = XWM_FRAME_CLOSE;
	}
	proto_add_int(out, res);
}

void handle(int from_pid, proto_t* in, void* p) {
	(void)p;
	int cmd = proto_read_int(in);
	proto_t out;
	proto_init(&out, NULL, 0);

	if(cmd == XWM_CNTL_DRAW_FRAME) { //draw frame
		draw_frame(in, &out);
	}
	else if(cmd == XWM_CNTL_DRAW_DESKTOP) { //draw desktop
		draw_desktop(in, &out);
	}
	else if(cmd == XWM_CNTL_GET_POS) { //get pos
		get_pos(in, &out);
	}

	ipc_send(from_pid, &out, in->id);
	proto_clear(&out);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	read_config(&_xwm, "/etc/x/xwm.conf");
	_xwm.title_h = 24;
	ipc_server(handle, NULL);
	return 0;
}
