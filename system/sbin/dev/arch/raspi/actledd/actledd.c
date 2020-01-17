#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <vfs.h>
#include <gpio.h>
#include <vdevice.h>
#include <syscall.h>
#include <dev/device.h>

static int mount(fsinfo_t* mnt_point, void* p) {
	(void)p;
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	strcpy(info.name, mnt_point->name);
	info.type = FS_TYPE_DEV;
	info.data = DEV_NULL;
	vfs_new_node(&info);

	if(vfs_mount(mnt_point, &info) != 0) {
		vfs_del(&info);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

static int actled_mount(fsinfo_t* info, void* p) {
	mount(info, p);
	return 0;
}

static int _gpio_fd = -1;

static int actled_write(int fd, int from_pid, fsinfo_t* info, const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)size;
	(void)p;

	if(((const char*)buf)[0] == 0)
		gpio_write(_gpio_fd, 47, 1); //1 for off
	else
		gpio_write(_gpio_fd, 47, 0); //o for on
	return 1;
}

int main(int argc, char** argv) {
	_gpio_fd = open("/dev/gpio", O_RDWR);
	if(_gpio_fd < 0)
		return -1;
	gpio_config(_gpio_fd, 47, 1);

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/actled";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "actled");
	dev.mount = actled_mount;
	dev.write = actled_write;

	device_run(&dev, mnt_point, FS_TYPE_DEV, NULL, 1);
	close(_gpio_fd);
	return 0;
}
