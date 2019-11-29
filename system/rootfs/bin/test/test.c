#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vfs.h>
#include <sdc.h>
#include <dev/device.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	char buf0[SDC_BLOCK_SIZE];
	char buf1[SDC_BLOCK_SIZE];
	memset(buf0, 'x', SDC_BLOCK_SIZE);
	sdc_write(0, buf0);
	sdc_read(0, buf1);

	uprintf("%s", buf1);
	return 0;
}
