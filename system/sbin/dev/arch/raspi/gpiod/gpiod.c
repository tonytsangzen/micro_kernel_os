#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <vfs.h>
#include <vdevice.h>
#include <dev/device.h>
#include "gpio_arch.h"

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

static int gpio_mount(fsinfo_t* info, mount_info_t* mnt_info, void* p) {
	mount(info, mnt_info, p);
	return 0;
}

static int gpio_fcntl(int fd, int from_pid, fsinfo_t* info, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)p;

	int gpio_num = proto_read_int(in);
	if(cmd == 0) { //0: config
		int v = proto_read_int(in);
		gpio_config(gpio_num, v);
	//	kprintf("gpio config n: %d, v: %d\n", gpio_num, v);
	}
	else if(cmd == 1) { //1: pull
		int v = proto_read_int(in);
		gpio_pull(gpio_num, v);
	//	kprintf("gpio pull n: %d, v: %d\n", gpio_num, v);
	}
	else if(cmd == 2) { //2: write
		int v = proto_read_int(in);
		gpio_write(gpio_num, v);
	//	kprintf("gpio write n: %d, v: %d\n", gpio_num, v);
	}
	else if(cmd == 3) { //3: read
		int v = gpio_read(gpio_num);
		proto_add_int(out, v);
	//	kprintf("gpio read n: %d\n", gpio_num);
	}
	return 0;
}

static int gpio_umount(fsinfo_t* info, void* p) {
	(void)p;
	vfs_umount(info);
	return 0;
}

int main(int argc, char** argv) {
	gpio_init();

	fsinfo_t mnt_point;
	const char* mnt_name = argc > 1 ? argv[1]: "/dev/gpio";
	vfs_create(mnt_name, &mnt_point, FS_TYPE_DEV);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "gpio");
	dev.mount = gpio_mount;
	dev.fcntl = gpio_fcntl;
	dev.umount = gpio_umount;

	mount_info_t mnt_info;
	strcpy(mnt_info.dev_name, dev.name);
	mnt_info.dev_index = 0;
	mnt_info.access = 0;

	device_run(&dev, &mnt_point, &mnt_info, NULL, 1);
	return 0;
}
