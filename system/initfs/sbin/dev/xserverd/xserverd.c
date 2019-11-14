#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <vfs.h>
#include <vdevice.h>
#include <graph/graph.h>
#include <svc_call.h>
#include <dev/device.h>
#include <shm.h>

typedef struct {
	int32_t fd;
	int32_t shm_id;
	grect_t rect;
} xview_t;

static int xserver_mount(fsinfo_t* mnt_point, mount_info_t* mnt_info, void* p) {
	(void)p;
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	strcpy(info.name, mnt_point->name);
	info.type = FS_TYPE_DEV;
	vfs_new_node(&info);

	if(vfs_mount(mnt_point, &info, mnt_info) != 0) {
		vfs_del(&info);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

static int xserver_umount(fsinfo_t* info, void* p) {
	(void)p;
	vfs_umount(info);
	return 0;
}

static int xserver_cntl(int fd, fsinfo_t* info, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)fd;
	(void)info;
	(void)in;
	(void)out;
	(void)p;
	(void)cmd;

	return 0;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xserver");
	dev.mount = xserver_mount;
	dev.cntl = xserver_cntl;
	dev.umount = xserver_umount;

	fsinfo_t dev_info;
	vfs_get("/dev", &dev_info);

	fsinfo_t mnt_point;
	memset(&mnt_point, 0, sizeof(fsinfo_t));
	strcpy(mnt_point.name, "xserver");
	mnt_point.type = FS_TYPE_DEV;

	vfs_new_node(&mnt_point);
	vfs_add(&dev_info, &mnt_point);

	mount_info_t mnt_info;
	strcpy(mnt_info.dev_name, dev.name);
	mnt_info.dev_index = 0;
	mnt_info.access = 0;

	device_run(&dev, &mnt_point, &mnt_info, NULL);
	return 0;
}
