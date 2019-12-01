#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vfs.h>
#include <sd.h>
#include <ext2fs.h>
#include <dev/device.h>

static void out(void* data, int32_t size) {
	char* buf = (char*)data;
	int32_t wr = 0;
	while(1) {
		if(size <= 0)
			break;

		int sz = write(1, buf, size);
		if(sz <= 0 && errno != EAGAIN)
			break;

		if(sz > 0) {
			size -= sz;
			wr += sz;
			buf += sz;
		}
	}
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: test <ext2 fname>\n");
		return -1;
	}

	ext2_t ext2;
	ext2_init(&ext2, sd_read, sd_write);

	int32_t sz;
	void* data = ext2_readfile(&ext2, argv[1], &sz);
	if(data != NULL) {
		out(data, sz);
		free(data);
	}

	ext2_quit(&ext2);
	return 0;
}
