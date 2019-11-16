#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <ipc.h>
#include <shm.h>
#include <graph/graph.h>

void handle(int from_pid, proto_t* in, void* p) {
	(void)p;

	int32_t shm_id = proto_read_int(in);
	int x = proto_read_int(in);
	int y = proto_read_int(in);
	int w = proto_read_int(in);
	int h = proto_read_int(in);
	int xw = proto_read_int(in);
	int xh = proto_read_int(in);

	void* gbuf = shm_map(shm_id);
	int res = -1;
	if(gbuf != NULL) {
		graph_t* g = graph_new(gbuf, xw, xh);
		box(g, x, y, w, h, 0xffffffff);

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

	ipc_server(handle, NULL);
	return 0;
}
