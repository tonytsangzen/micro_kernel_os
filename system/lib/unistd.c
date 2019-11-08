#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <svc_call.h>
#include <proto.h>
#include <vfs.h>
#include <ipc.h>
#include <string.h>
#include <ramfs.h>

int getpid(void) {
	return svc_call0(SYS_GET_PID);
}

int fork(void) {
	return svc_call0(SYS_FORK);
}

unsigned int sleep(unsigned int seconds) {
	if(seconds == 0)
		svc_call0(SYS_YIELD);
	return 0;
}

int read(int fd, void* buf, uint32_t size) {
	fsinfo_t info;

	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) != 0)
		return -1;

	int offset = svc_call1(SYS_VFS_PROC_TELL, fd);
	if(offset < 0)
		offset = 0;
	
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_READ);
	proto_add(&in, &info, sizeof(fsinfo_t));
	proto_add_int(&in, size);
	proto_add_int(&in, offset);

	int res = -1;
	if(ipc_call(mount.pid, &in, &out) == 0) {
		int rd = proto_read_int(&out);
		res = rd;
		if(rd > 0) {
			memcpy(buf, proto_read(&out, NULL), rd);
			offset += rd;
			svc_call2(SYS_VFS_PROC_SEEK, fd, offset);
		}
	}
	proto_clear(&in);
	proto_clear(&out);

	return res;
}

int write(int fd, const void* buf, uint32_t size) {
	fsinfo_t info;

	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) != 0)
		return -1;

	int offset = svc_call1(SYS_VFS_PROC_TELL, fd);
	if(offset < 0)
		offset = 0;
	
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_WRITE);
	proto_add(&in, &info, sizeof(fsinfo_t));
	proto_add(&in, buf, size);
	proto_add_int(&in, offset);

	int res = -1;
	if(ipc_call(mount.pid, &in, &out) == 0) {
		int r = proto_read_int(&out);
		res = r;
		if(r > 0) {
			offset += r;
			svc_call2(SYS_VFS_PROC_SEEK, fd, offset);
		}
	}
	proto_clear(&in);
	proto_clear(&out);

	return res;
}

void exec_initfs(const char* fname) {
	ramfs_t ramfs;
	const char* initrd = (const char*)svc_call0(SYS_INITRD);
	ramfs_open(initrd, &ramfs);
	int sz;
	const char* elf = ramfs_read(&ramfs, fname, &sz);
	svc_call3(SYS_EXEC_ELF, (int32_t)fname, (int32_t)elf, sz);
}

int exec(const char* cmd_line) {
	fsinfo_t info;

	if(vfs_get(cmd_line, &info) != 0 || info.size == 0)
		return -1;

	void* buf = malloc(info.size);
	if(buf == NULL)
		return -1;

	int fd = open(cmd_line, 0);
	int sz = read(fd, buf, info.size);
	if(sz != (int)info.size) {
		free(buf);
		return -1;
	}
	close(fd);
	svc_call3(SYS_EXEC_ELF, (int32_t)cmd_line, (int32_t)buf, sz);
	free(buf);
	return 0;
}

char* getcwd(char* buf, uint32_t size) {
	svc_call2(SYS_PROC_GET_CWD, (int32_t)buf, (int32_t)size);
	return buf;
}

int chdir(const char* path) {
	return svc_call1(SYS_PROC_SET_CWD, (int32_t)path);
}
