#ifndef VFS_H
#define VFS_H

#include <fsinfo.h>

int vfs_new_node(const char* name, int type, fsinfo_t* info);
int vfs_add(fsinfo_t* to, fsinfo_t* info);
int vfs_del(fsinfo_t* info);
int vfs_get_info(const char* fname, fsinfo_t* info);
int vfs_set_info(fsinfo_t* info);
int vfs_mount(fsinfo_t* mount_to, fsinfo_t* info, int access);
int vfs_umount(fsinfo_t* info);

#endif
