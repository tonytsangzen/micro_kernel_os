#include <sd.h>
#include <dev/device.h>
#include <syscall.h>
#include <partition.h>
#include <string.h>

#define EXT2_BLOCK_SIZE 1024
static partition_t _partition;

static int32_t sd_read_sector(int32_t sector, void* buf) {
	if(syscall2(SYS_DEV_BLOCK_READ, DEV_SD, sector) != 0)
		return -1;
	while(1) {
		if(syscall2(SYS_DEV_BLOCK_READ_DONE, DEV_SD, (int32_t)buf)  == 0)
			break;
	}
	return 0;
}

static int32_t sd_write_sector(int32_t sector, const void* buf) {
	if(syscall3(SYS_DEV_BLOCK_WRITE, DEV_SD, sector, (int32_t)buf) != 0)
		return -1;
	while(1) {
		if(syscall1(SYS_DEV_BLOCK_WRITE_DONE, DEV_SD)  == 0)
			break;
	}
	return 0;
}

int32_t sd_read(int32_t block, void* buf) {
  int32_t n = EXT2_BLOCK_SIZE/512;
  int32_t sector = block * n + _partition.start_sector;
  char* p = (char*)buf;

	while(n > 0) {
		//kprintf("raw_sec: %d, sec: %d, p: %d\n", block*n, sector, _partition.start_sector);
		if(sd_read_sector(sector, p) != 0) {
			kprintf("ERR: raw_sec: %d, sec: %d, p: %d\n", block*n, sector, _partition.start_sector);
			return -1;
		}
		sector++;
		n--;
		p += 512;
	}
	return 0;
}

int32_t sd_write(int32_t block, const void* buf) {
  int32_t n = EXT2_BLOCK_SIZE/512;
  int32_t sector = block * n + _partition.start_sector;
  const char* p = (char*)buf;

	while(n > 0) {
		if(sd_write_sector(sector, p) != 0)
			return -1;
		sector++;
		n--;
		p += 512;
	}
	return 0;
}

#define PARTITION_MAX 4
static partition_t _partitions[PARTITION_MAX];

int32_t read_partition(void) {
	uint8_t sector[512];
	if(sd_read_sector(0, sector) != 0)
		return -1;
	//check magic 
	if(sector[510] != 0x55 || sector[511] != 0xAA) 
		return -1;

	memcpy(_partitions, sector+0x1BE, sizeof(partition_t)*PARTITION_MAX);
	return 0;
}

int32_t partition_get(uint32_t id, partition_t* p) {
	if(id >= PARTITION_MAX)
		return -1;
	memcpy(p, &_partitions[id], sizeof(partition_t));
	return 0;
}

int32_t sd_init(void) {
	if(read_partition() != 0 || partition_get(1, &_partition) != 0) {
		memset(&_partition, 0, sizeof(partition_t));
	}
	return 0;
}
