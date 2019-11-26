#ifndef VFS_H
#define VFS_H

#include <fsinfo.h>

const char* vfs_fullname(const char* fname);

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

void* vfs_readfile(const char* fname, int* sz);

#endif
