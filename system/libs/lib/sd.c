#include <sd.h>
#include <dev/device.h>
#include <syscall.h>

int32_t sd_read(int32_t block, void* buf) {
	if(syscall2(SYS_DEV_BLOCK_READ, DEV_SD, block) != 0)
		return -1;
	while(1) {
		if(syscall2(SYS_DEV_BLOCK_READ_DONE, DEV_SD, (int32_t)buf)  == 0)
			break;
	}
	return 0;
}

int32_t sd_write(int32_t block, const void* buf) {
	if(syscall3(SYS_DEV_BLOCK_WRITE, DEV_SD, block, (int32_t)buf) != 0)
		return -1;

	while(1) {
		if(syscall1(SYS_DEV_BLOCK_WRITE_DONE, DEV_SD)  == 0)
			break;
	}
	return 0;
}
