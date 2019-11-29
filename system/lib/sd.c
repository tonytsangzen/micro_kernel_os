#include <sd.h>
#include <dev/device.h>
#include <syscall.h>

int sd_read(int block, void* buf) {
	if(syscall2(SYS_DEV_BLOCK_READ, DEV_SD, block) != 0)
		return -1;

	while(1) {
		if(syscall2(SYS_DEV_BLOCK_READ_DONE, DEV_SD, (int32_t)buf)  == 0)
			break;
	}
	return 0;
}

int sd_write(int block, const void* buf) {
	if(syscall3(SYS_DEV_BLOCK_WRITE, DEV_SD, block, (int32_t)buf) != 0)
		return -1;

	while(1) {
		if(syscall1(SYS_DEV_BLOCK_WRITE_DONE, DEV_SD)  == 0)
			break;
	}
	return 0;
}
