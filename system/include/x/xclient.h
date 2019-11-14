#ifndef XCLIENT_H
#define XCLIENT_H

#include <graph/graph.h>

typedef struct {
	int shm_id;
	grect_t r;
} xinfo_t;

typedef struct {
	int fd;
	xinfo_t xinfo;
	graph_t* g;
} x_t;

x_t*     x_open(int x, int y, int w, int h);
graph_t* x_graph(x_t* x);
void     x_close(x_t* x);

#endif
