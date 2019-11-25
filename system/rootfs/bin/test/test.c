#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vfs.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int sz;
	void* data = vfs_readfile("/data/laptop.png", &sz);
	if(data == NULL)
		return -1;
	free(data);
	return 0;
}
