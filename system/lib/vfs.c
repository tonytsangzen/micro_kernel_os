#include <vfs.h>
#include <string.h>
#include <svc_call.h>

int vfs_new_node(const char* name, int type, fsinfo_t* info) {
	int res = svc_call1(SYS_VFS_NEW_NODE, (int32_t)info);
	if(res < 0)
		return -1;

	info->type = type;
	strcpy(info->name, name);
	return vfs_set_info(info);
}

int vfs_get_info(const char* fname, fsinfo_t* info) {
	return svc_call2(SYS_VFS_GET_INFO, (int32_t)fname, (int32_t)info);
}

int vfs_set_info(fsinfo_t* info) {
	return svc_call1(SYS_VFS_SET_INFO, (int32_t)info);
}

int vfs_add(fsinfo_t* to, fsinfo_t* info) {
	return svc_call2(SYS_VFS_ADD, (int32_t)to, (int32_t)info);
}

int vfs_del(fsinfo_t* info) {
	return svc_call1(SYS_VFS_DEL, (int32_t)info);
}

int vfs_mount(fsinfo_t* mount_to, fsinfo_t* info, int access) {
	return svc_call3(SYS_VFS_MOUNT, (int32_t)mount_to, (int32_t)info, access);
}

int vfs_umount(fsinfo_t* info) {
	return svc_call1(SYS_VFS_UMOUNT, (int32_t)info);
}


