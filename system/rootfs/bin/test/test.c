#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vfs.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	while(1) {
		usleep(100000);
		printf("tic.\n");
	}
	return 0;
}
