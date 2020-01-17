#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <vfs.h>
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

static int null_mount(fsinfo_t* info, void* p) {
	mount(info, p);
	return 0;
}

static int null_read(int fd, int from_pid, fsinfo_t* info, void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)buf;
	(void)size;
	(void)offset;
	(void)p;
	return 0;	
}

static int null_write(int fd, int from_pid, fsinfo_t* info, const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)buf;
	(void)offset;
	(void)p;
	return size;
}

static int null_umount(fsinfo_t* info, void* p) {
	(void)p;
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/null";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "null");
	dev.mount = null_mount;
	dev.read = null_read;
	dev.write = null_write;
	dev.umount = null_umount;

	device_run(&dev, mnt_point, FS_TYPE_DEV, NULL, 1);
	return 0;
}
