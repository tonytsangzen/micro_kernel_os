#include <vfs.h>
#include <string.h>
#include <svc_call.h>

int vfs_new_node(const char* name, int type, fsinfo_t* info) {
	int res = svc_call1(SYS_VFS_NEW_NODE, (int32_t)info);
	if(res < 0)
		return -1;

	info->type = type;
	strcpy(info->name, name);
	return vfs_set(info);
}

int vfs_get(const char* fname, fsinfo_t* info) {
	return svc_call2(SYS_VFS_GET, (int32_t)fname, (int32_t)info);
}

int vfs_first_kid(fsinfo_t* info, fsinfo_t* ret) {
	return svc_call2(SYS_VFS_FKID, (int32_t)info, (int32_t)ret);
}

int vfs_next(fsinfo_t* info, fsinfo_t* ret) {
	return svc_call2(SYS_VFS_NEXT, (int32_t)info, (int32_t)ret);
}

int vfs_father(fsinfo_t* info, fsinfo_t* ret) {
	return svc_call2(SYS_VFS_FATHER, (int32_t)info, (int32_t)ret);
}

int vfs_set(fsinfo_t* info) {
	return svc_call1(SYS_VFS_SET, (int32_t)info);
}

int vfs_add(fsinfo_t* to, fsinfo_t* info) {
	return svc_call2(SYS_VFS_ADD, (int32_t)to, (int32_t)info);
}

int vfs_del(fsinfo_t* info) {
	return svc_call1(SYS_VFS_DEL, (int32_t)info);
}

int vfs_get_mount(fsinfo_t* info, mount_t* mount) {
	return svc_call2(SYS_VFS_GET_MOUNT, (int32_t)info, (int32_t)mount);
}

int vfs_mount(fsinfo_t* mount_to, fsinfo_t* info, int access) {
	return svc_call3(SYS_VFS_MOUNT, (int32_t)mount_to, (int32_t)info, access);
}

int vfs_umount(fsinfo_t* info) {
	return svc_call1(SYS_VFS_UMOUNT, (int32_t)info);
}

int vfs_open(int pid, fsinfo_t* info, int wr) {
	return svc_call3(SYS_VFS_OPEN, (int32_t)pid, (int32_t)info, (int32_t)wr);
}

int vfs_close(int pid, int fd) {
	return svc_call2(SYS_VFS_CLOSE, (int32_t)pid, (int32_t)fd);
}

int vfs_seek(int fd, int offset, int whence) {
	return svc_call3(SYS_VFS_SEEK, (int32_t)fd,(int32_t)offset, (int32_t)whence);
}

int vfs_get_by_fd(int fd, fsinfo_t* info) {
	return svc_call2(SYS_VFS_GET_BY_FD, (int32_t)fd, (int32_t)info);
}
