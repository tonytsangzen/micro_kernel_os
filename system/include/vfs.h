#ifndef VFS_H
#define VFS_H

#include <fsinfo.h>

enum {
	FS_CMD_NONE = 0,
	FS_CMD_OPEN,
	FS_CMD_CLOSE,
	FS_CMD_READ,
	FS_CMD_WRITE,
	FS_CMD_SEEK
};

int vfs_new_node(fsinfo_t* info);
int vfs_add(fsinfo_t* to, fsinfo_t* info);
int vfs_del(fsinfo_t* info);
int vfs_get(const char* fname, fsinfo_t* info);
int vfs_get_by_fd(int fd, fsinfo_t* info);
int vfs_first_kid(fsinfo_t* info, fsinfo_t* ret);
int vfs_next(fsinfo_t* info, fsinfo_t* ret);
int vfs_father(fsinfo_t* info, fsinfo_t* ret);
int vfs_set(fsinfo_t* info);
int vfs_get_mount(fsinfo_t* info, mount_t* mount);

int vfs_mount(fsinfo_t* mount_to, fsinfo_t* info, mount_info_t* mnt_info);
void vfs_mount_wait(const char* fname, int pid);
int vfs_umount(fsinfo_t* info);

int vfs_open(int pid, fsinfo_t* info, int wr);
int vfs_close(int fd);
int vfs_seek(int fd, int offset, int whence);
int vfs_dup2(int from, int to);

#endif
