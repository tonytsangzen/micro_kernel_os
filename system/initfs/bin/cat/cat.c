#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char** argv) {
	if(argc != 2) {
		printf("  Usage: cat <file>\n");
		return -1;
	}

	int fd = open(argv[1], 0);
	if(fd < 0) {
		printf("'%s' not found!\n", argv[1]);
		return -1;
	}

	while(1) {
		char buf[1024];
		int sz = read(fd, buf, 1024);
		if(sz <= 0 && errno != EAGAIN)
			break;

		if(sz > 0)
			write(1, buf, sz);
		sleep(0);
	}

	return 0;
}
