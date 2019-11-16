#ifndef XCLIENT_H
#define XCLIENT_H

#include <graph/graph.h>
#include <x/xcntl.h>
#include <x/xevent.h>

typedef struct {
	int fd;
	xinfo_t xinfo;
	graph_t* g;
} x_t;

x_t*     x_open(int x, int y, int w, int h, const char* title);
graph_t* x_graph(x_t* x);
int      x_update(x_t* x);
void     x_close(x_t* x);
int      x_get_event(x_t* x, xevent_t* ev);

#endif
