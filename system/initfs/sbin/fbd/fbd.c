#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <vfs.h>
#include <vdevice.h>
#include <dev/fbinfo.h>
#include <svc_call.h>
#include <dev/device.h>
#include <shm.h>

typedef struct {
	void* data;
	uint32_t size;
	int32_t shm_id;
} fb_dma_t;

static int fb_mount(fsinfo_t* mnt_point, mount_info_t* mnt_info, void* p) {
	(void)p;
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	strcpy(info.name, mnt_point->name);
	info.type = FS_TYPE_DEV;
	info.data = DEV_FRAMEBUFFER;
	vfs_new_node(&info);

	if(vfs_mount(mnt_point, &info, mnt_info) != 0) {
		vfs_del(&info);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

static int fb_flush(fsinfo_t* info, void* p) {
	fb_dma_t* dma = (fb_dma_t*)p;
	return svc_call3(SYS_DEV_WRITE, (int32_t)info->data, (int32_t)dma->data, dma->size);
}

static int fb_dma(fsinfo_t* info, int* size, void* p) {
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;
	*size = dma->size;
	return dma->shm_id;
}

static int fb_umount(fsinfo_t* info, void* p) {
	(void)p;
	vfs_umount(info);
	return 0;
}

static int fb_cntl(fsinfo_t* info, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)info;
	(void)in;
	(void)p;

	if(cmd == CNTL_INFO) {
		fbinfo_t fbinfo;
		svc_call3(SYS_DEV_OP, DEV_FRAMEBUFFER, DEV_OP_INFO, (int32_t)&fbinfo);
		proto_add(out, &fbinfo, sizeof(fbinfo_t));	
	}
	return 0;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	fbinfo_t fbinfo;
	svc_call3(SYS_DEV_OP, DEV_FRAMEBUFFER, DEV_OP_INFO, (int32_t)&fbinfo);
	uint32_t sz = fbinfo.width*fbinfo.height*4;

	fb_dma_t dma;
	dma.shm_id = shm_alloc(sz, 1);
	if(dma.shm_id <= 0)
		return -1;
	dma.size = sz;
	dma.data = shm_map(dma.shm_id);
	if(dma.data == NULL)
		return -1;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "framebuffer");
	dev.mount = fb_mount;
	dev.dma = fb_dma;
	dev.flush = fb_flush;
	dev.cntl = fb_cntl;
	dev.umount = fb_umount;

	fsinfo_t dev_info;
	vfs_get("/dev", &dev_info);

	fsinfo_t mnt_point;
	memset(&mnt_point, 0, sizeof(fsinfo_t));
	strcpy(mnt_point.name, "fb0");
	mnt_point.type = FS_TYPE_DEV;

	vfs_new_node(&mnt_point);
	vfs_add(&dev_info, &mnt_point);

	mount_info_t mnt_info;
	strcpy(mnt_info.dev_name, dev.name);
	mnt_info.dev_index = 0;
	mnt_info.access = 0;

	device_run(&dev, &mnt_point, &mnt_info, &dma);

	shm_unmap(dma.shm_id);
	return 0;
}
