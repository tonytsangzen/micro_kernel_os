#include <sd.h>
#include <dev/device.h>
#include <syscall.h>

#define EXT2_BLOCK_SIZE 1024

int32_t sd_read(int32_t block, void* buf) {
  int32_t n = EXT2_BLOCK_SIZE/512;
  int32_t sector = block * n;
  char* p = (char*)buf;

	while(n > 0) {
		if(syscall2(SYS_DEV_BLOCK_READ, DEV_SD, sector) != 0)
			return -1;
		while(1) {
			if(syscall2(SYS_DEV_BLOCK_READ_DONE, DEV_SD, (int32_t)p)  == 0)
				break;
		}
		sector++;
		n--;
		p += 512;
	}
	return 0;
}

int32_t sd_write(int32_t block, const void* buf) {
  int32_t n = EXT2_BLOCK_SIZE/512;
  int32_t sector = block * n;
  const char* p = (char*)buf;

	while(n > 0) {
		if(syscall3(SYS_DEV_BLOCK_WRITE, DEV_SD, sector, (int32_t)buf) != 0)
			return -1;
		while(1) {
			if(syscall1(SYS_DEV_BLOCK_WRITE_DONE, DEV_SD)  == 0)
				break;
		}
		sector++;
		n--;
		p += 512;
	}
	return 0;
}
