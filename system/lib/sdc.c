#include <sdc.h>
#include <dev/device.h>
#include <syscall.h>

int sdc_read(int block, void* buf) {
	if(syscall2(SYS_DEV_BLOCK_READ, DEV_SDC, block) != 0)
		return -1;

	while(1) {
		if(syscall2(SYS_DEV_BLOCK_READ_DONE, DEV_SDC, (int32_t)buf)  == 0)
			break;
	}
	return 0;
}

int sdc_write(int block, const void* buf) {
	if(syscall3(SYS_DEV_BLOCK_WRITE, DEV_SDC, block, (int32_t)buf) != 0)
		return -1;

	while(1) {
		if(syscall1(SYS_DEV_BLOCK_WRITE_DONE, DEV_SDC)  == 0)
			break;
	}
	return 0;
}
