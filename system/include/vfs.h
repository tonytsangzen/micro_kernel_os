#ifndef VFS_H
#define VFS_H

#include <fsinfo.h>

int vfs_new_node(const char* name, int type, fsinfo_t* info);
int vfs_add(fsinfo_t* to, fsinfo_t* info);
int vfs_del(fsinfo_t* info);
int vfs_get(const char* fname, fsinfo_t* info);
int vfs_first_kid(fsinfo_t* info, fsinfo_t* ret);
int vfs_next(fsinfo_t* info, fsinfo_t* ret);
int vfs_father(fsinfo_t* info, fsinfo_t* ret);
int vfs_set(fsinfo_t* info);
int vfs_get_mount(fsinfo_t* info, mount_t* mount);
int vfs_mount(fsinfo_t* mount_to, fsinfo_t* info, int access);
int vfs_umount(fsinfo_t* info);
int vfs_open(int pid, fsinfo_t* info, int wr);
int vfs_close(int pid, int fd);
int vfs_seek(int fd, int offset, int whence);

#endif
