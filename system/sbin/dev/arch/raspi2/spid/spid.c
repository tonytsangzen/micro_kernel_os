#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <vfs.h>
#include <vdevice.h>
#include <dev/device.h>
#include "../lib/spi_arch.h"
#include "../lib/gpio_arch.h"

static int mount(fsinfo_t* mnt_point, void* p) {
	(void)p;
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	strcpy(info.name, mnt_point->name);
	info.type = FS_TYPE_DEV;
	vfs_new_node(&info);

	if(vfs_mount(mnt_point, &info) != 0) {
		vfs_del(&info);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

static int spi_mount(fsinfo_t* info, void* p) {
	mount(info, p);
	return 0;
}

static int spi_umount(fsinfo_t* info, void* p) {
	(void)p;
	vfs_umount(info);
	return 0;
}

static int spi_write(int fd, int from_pid, fsinfo_t* info, const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)size;
	(void)p;
	(void)buf;

	spi_arch_activate(SPI_ACTIVATE);
	int i;
	uint8_t* wr = (uint8_t*)buf;
	for(i=0; i<size; i++) {
		spi_arch_transfer(wr[i]);
	}
	spi_arch_activate(SPI_DEACTIVATE);
	return i;
}

int main(int argc, char** argv) {
	gpio_arch_init();
	spi_arch_init(4);
	spi_arch_select(SPI_SELECT_0);

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/spi";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "spi");
	dev.mount = spi_mount;
	dev.write = spi_write;
	dev.umount = spi_umount;

	device_run(&dev, mnt_point, FS_TYPE_DEV, NULL, 1);
	return 0;
}
