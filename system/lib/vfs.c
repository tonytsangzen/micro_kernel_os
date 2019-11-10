#include <vfs.h>
#include <string.h>
#include <svc_call.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <tstr.h>

int vfs_new_node(fsinfo_t* info) {
	return svc_call1(SYS_VFS_NEW_NODE, (int32_t)info);
}

int vfs_get(const char* fname, fsinfo_t* info) {
	tstr_t* fullname = tstr_new("");
	if(fname[0] == '/') {
		tstr_cpy(fullname, fname);
	}
	else {
		char* pwd = (char*)malloc(FS_FULL_NAME_MAX);
		getcwd(pwd, FS_FULL_NAME_MAX-1);
		tstr_cpy(fullname, pwd);
		free(pwd);
		if(pwd[1] != 0)
			tstr_addc(fullname, '/');
		tstr_add(fullname, fname);
	}

	fname = CS(fullname);
	int res = svc_call2(SYS_VFS_GET, (int32_t)fname, (int32_t)info);
	tstr_free(fullname);
	return res;
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

int vfs_mount(fsinfo_t* mount_to, fsinfo_t* info, mount_info_t* mnt_info) {
	return svc_call3(SYS_VFS_MOUNT, (int32_t)mount_to, (int32_t)info, (int32_t)mnt_info);
}

void vfs_mount_wait(const char* fname, int pid) {
	while(1) {
		fsinfo_t info;
		mount_t mnt_info;

		if(vfs_get(fname, &info) != 0)
			continue;
		if(vfs_get_mount(&info, &mnt_info) != 0)
			continue;

		if(mnt_info.pid == pid)
			break;
		sleep(0);
	}
}

int vfs_umount(fsinfo_t* info) {
	return svc_call1(SYS_VFS_UMOUNT, (int32_t)info);
}

int vfs_open(int pid, fsinfo_t* info, int wr) {
	return svc_call3(SYS_VFS_OPEN, (int32_t)pid, (int32_t)info, (int32_t)wr);
}

int vfs_close(int fd) {
	return svc_call1(SYS_VFS_PROC_CLOSE, (int32_t)fd);
}

int vfs_seek(int fd, int offset, int whence) {
	return svc_call3(SYS_VFS_PROC_SEEK, (int32_t)fd,(int32_t)offset, (int32_t)whence);
}

int vfs_get_by_fd(int fd, fsinfo_t* info) {
	return svc_call2(SYS_VFS_PROC_GET_BY_FD, (int32_t)fd, (int32_t)info);
}

int vfs_dup2(int from, int to) {
	return svc_call2(SYS_VFS_PROC_DUP2, (int32_t)from, (int32_t)to);
}

int vfs_dup(int from) {
	return svc_call1(SYS_VFS_PROC_DUP, (int32_t)from);
}
