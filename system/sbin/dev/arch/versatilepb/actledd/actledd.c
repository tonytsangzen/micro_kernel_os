#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <vfs.h>
#include <vdevice.h>
#include <syscall.h>
#include <dev/device.h>

static int mount(fsinfo_t* mnt_point, mount_info_t* mnt_info, void* p) {
	(void)p;
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	strcpy(info.name, mnt_point->name);
	info.type = FS_TYPE_DEV;
	info.data = DEV_NULL;
	vfs_new_node(&info);

	if(vfs_mount(mnt_point, &info, mnt_info) != 0) {
		vfs_del(&info);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

static int actled_mount(fsinfo_t* info, mount_info_t* mnt_info, void* p) {
	mount(info, mnt_info, p);
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

	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_int(&in, 47); //gpio 47

	if(((const char*)buf)[0] == 0)
		proto_add_int(&in, 1); //1 for off
	else
		proto_add_int(&in, 0); //0 for on

	int res = fcntl_raw(_gpio_fd, 2, &in, NULL); //2 for write
	proto_clear(&in);
	if(res != 0)
		return 0;
	return 1;
}

static int actled_umount(fsinfo_t* info, void* p) {
	(void)p;
	vfs_umount(info);
	return 0;
}

int main(int argc, char** argv) {
	_gpio_fd = open("/dev/gpio", O_RDWR);
	if(_gpio_fd < 0)
		return -1;
	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_int(&in, 47); //gpio 47
	proto_add_int(&in, 1); //1 for output mode
	int res = fcntl_raw(_gpio_fd, 0, &in, NULL); //0 for config
	proto_clear(&in);
	if(res != 0) //
		return -1;

	fsinfo_t mnt_point;
	const char* mnt_name = argc > 1 ? argv[1]: "/dev/actled";
	vfs_create(mnt_name, &mnt_point, FS_TYPE_DEV);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "actled");
	dev.mount = actled_mount;
	dev.write = actled_write;
	dev.umount = actled_umount;

	mount_info_t mnt_info;
	strcpy(mnt_info.dev_name, dev.name);
	mnt_info.dev_index = 0;
	mnt_info.access = 0;

	device_run(&dev, &mnt_point, &mnt_info, NULL, 1);
	close(_gpio_fd);
	return 0;
}
