#ifndef VDEVICE_H
#define VDEVICE_H

#include <fsinfo.h>

typedef struct {
	char name[FS_NODE_NAME_MAX];
	int (*open)(fsinfo_t* info, int oflag, void* p);
	int (*close)(fsinfo_t* info, void* p);
	int (*read)(fsinfo_t* info, void* buf, int size, int offset, void* p);
	int (*write)(fsinfo_t* info, const void* buf, int size, int offset, void* p);
	int (*dma)(fsinfo_t* info, int* size, void* p);
	int (*flush)(fsinfo_t* info, void* p);
	int (*mount)(fsinfo_t* mnt_point, mount_info_t* mnt_info, void* p);
	int (*umount)(fsinfo_t* mnt_point, void* p);
} vdevice_t;

extern int device_run(vdevice_t* dev, fsinfo_t* mnt_point, mount_info_t* mnt_info, void* p);

#endif
