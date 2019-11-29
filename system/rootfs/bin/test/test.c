#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vfs.h>
#include <sd.h>
#include <dev/device.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	char buf0[SD_BLOCK_SIZE];
	char buf1[SD_BLOCK_SIZE];
	memset(buf0, 'x', SD_BLOCK_SIZE);
	sd_write(0, buf0);
	sd_read(0, buf1);

	uprintf("%s", buf1);
	return 0;
}
