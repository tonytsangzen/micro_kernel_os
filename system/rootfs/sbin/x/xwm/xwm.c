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

typedef struct {
	font_t* font;
	uint32_t desk_fg_color;
	uint32_t desk_bg_color;
	uint32_t fg_color;
	uint32_t bg_color;
	uint32_t top_bg_color;
	uint32_t top_fg_color;
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

static void draw_desktop(graph_t* g) {
	clear(g, _xwm.desk_bg_color);
	//background pattern
	int32_t x, y;
	for(y=10; y<(int32_t)g->h; y+=10) {
		for(x=0; x<(int32_t)g->w; x+=10) {
			pixel(g, x, y, _xwm.desk_fg_color);
		}
	}
}

static void draw_frame(graph_t* g, proto_t* in) {
	xinfo_t info;
	proto_read_to(in, &info, sizeof(xinfo_t));

	box(g, info.r.x, info.r.y, info.r.w, info.r.h, _xwm.fg_color);//win box
	fill(g, info.r.x, info.r.y-20, info.r.w, 20, _xwm.bg_color);//title box
	box(g, info.r.x, info.r.y-20, info.r.w, 20, _xwm.fg_color);//title box
	box(g, info.r.x+info.r.w-20, info.r.y-20, 20, 20, _xwm.fg_color);//close box
	draw_text(g, info.r.x+10, info.r.y-20+2, info.title, _xwm.font, _xwm.fg_color);//title
}

void handle(int from_pid, proto_t* in, void* p) {
	(void)p;

	int cmd = proto_read_int(in);
	int shm_id = proto_read_int(in);
	int xw = proto_read_int(in);
	int xh = proto_read_int(in);

	void* gbuf = shm_map(shm_id);
	int res = -1;
	if(gbuf != NULL) {
		graph_t* g = graph_new(gbuf, xw, xh);

		if(cmd == 0) { //draw frame
			draw_frame(g, in);
		}
		else if(cmd == 1) {
			draw_desktop(g);
		}

		graph_free(g);
		shm_unmap(shm_id);
		res = 0;
	}

	proto_t out;
	proto_init(&out, NULL, 0);
	proto_add_int(&out, res);
	ipc_send(from_pid, &out, in->id);
	proto_clear(&out);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	read_config(&_xwm, "/etc/x/xwm.conf");
	ipc_server(handle, NULL);
	return 0;
}
