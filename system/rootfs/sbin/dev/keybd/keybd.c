#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <vfs.h>
#include <vdevice.h>
#include <syscall.h>
#include <dev/device.h>

static int keyb_mount(fsinfo_t* mnt_point, mount_info_t* mnt_info, void* p) {
	(void)p;
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	strcpy(info.name, mnt_point->name);
	info.type = FS_TYPE_DEV;
	info.data = DEV_KEYB;
	vfs_new_node(&info);

	if(vfs_mount(mnt_point, &info, mnt_info) != 0) {
		vfs_del(&info);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

static int keyb_block(fsinfo_t* info,  void* p) {
	(void)p;
	(void)info;
	return syscall1(SYS_BLOCK_ON, DEV_KEYB);
}

static int keyb_read(int fd, int from_pid, fsinfo_t* info, void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	int res = syscall3(SYS_DEV_CHAR_READ, (int32_t)info->data, (int32_t)buf, size);
	if(res == 0)  {
		return ERR_RETRY;
	}
	return res;
}

static int keyb_umount(fsinfo_t* info, void* p) {
	(void)p;
	vfs_umount(info);
	return 0;
}

int main(int argc, char** argv) {
	fsinfo_t mnt_point;
	const char* mnt_name = argc > 1 ? argv[1]: "/dev/keyb0";
	vfs_create(mnt_name, &mnt_point, FS_TYPE_DEV);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "keyb");
	dev.mount = keyb_mount;
	dev.block = keyb_block;
	dev.read = keyb_read;
	dev.umount = keyb_umount;

	mount_info_t mnt_info;
	strcpy(mnt_info.dev_name, dev.name);
	mnt_info.dev_index = 0;
	mnt_info.access = 0;

	device_run(&dev, &mnt_point, &mnt_info, NULL, 1);
	return 0;
}
