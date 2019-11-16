#ifndef VDEVICE_H
#define VDEVICE_H

#include <fsinfo.h>
#include <proto.h>

typedef struct {
	char name[FS_NODE_NAME_MAX];
	int (*open)(int fd, int from_pid, fsinfo_t* info, int oflag, void* p);
	int (*close)(int fd, int from_pid, fsinfo_t* info, void* p);
	int (*read)(int fd, int from_pid, fsinfo_t* info, void* buf, int size, int offset, void* p);
	int (*write)(int fd, int from_pid, fsinfo_t* info, const void* buf, int size, int offset, void* p);
	int (*dma)(int fd, int from_pid, fsinfo_t* info, int* size, void* p);
	int (*flush)(int fd, int from_pid, fsinfo_t* info, void* p);
	int (*cntl)(int fd, int from_pid, fsinfo_t* info, int cmd, proto_t* in, proto_t* out, void* p);
	int (*mount)(fsinfo_t* mnt_point, mount_info_t* mnt_info, void* p);
	int (*umount)(fsinfo_t* mnt_point, void* p);
	int (*loop_step)(void* p);
} vdevice_t;

extern int device_run(vdevice_t* dev, fsinfo_t* mnt_point, mount_info_t* mnt_info, void* p);

#endif
