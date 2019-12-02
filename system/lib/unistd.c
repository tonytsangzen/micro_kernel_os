#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <syscall.h>
#include <proto.h>
#include <vfs.h>
#include <ipc.h>
#include <string.h>
#include <mstr.h>
#include <ramfs.h>
#include <errno.h>

int errno = ENONE;

int getpid(void) {
	return syscall0(SYS_GET_PID);
}

int fork(void) {
	return syscall0(SYS_FORK);
}

int usleep(unsigned int usecs) {
	if(usecs == 0)
		syscall0(SYS_YIELD);
	else
		syscall1(SYS_SLEEP, usecs / 1000);
	return 0;
}

unsigned int sleep(unsigned int seconds) {
	usleep(seconds * 1000 * 1000);
	return 0;
}

static int read_pipe(fsinfo_t* info, void* buf, uint32_t size) {
	int res = syscall3(SYS_PIPE_READ, (int32_t)info, (int32_t)buf, (int32_t)size);
	if(res == 0) { // pipe empty, do retry
		errno = EAGAIN;
		return -1;
	}
	if(res > 0) {
		return res;
	}
	return 0; //res < 0 , pipe closed, return 0.
}

int read(int fd, void* buf, uint32_t size) {
	errno = ENONE;
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

	if(info.type == FS_TYPE_PIPE)
		return read_pipe(&info, buf, size);
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) != 0)
		return -1;

	int offset = syscall1(SYS_VFS_PROC_TELL, fd);
	if(offset < 0)
		offset = 0;
	
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_READ);
	proto_add_int(&in, fd);
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
			syscall2(SYS_VFS_PROC_SEEK, fd, offset);
		}
		if(res == ERR_RETRY) {
			errno = EAGAIN;
			res = -1;
		}
	}
	proto_clear(&in);
	proto_clear(&out);

	return res;
}

static int write_pipe(fsinfo_t* info, const void* buf, uint32_t size) {
	errno = ENONE;
	int res = syscall3(SYS_PIPE_WRITE, (int32_t)info, (int32_t)buf, (int32_t)size);
	if(res == 0) { // pipe not empty, do retry
		errno = EAGAIN;
		return -1;
	}
	if(res > 0)
		return res;
	return 0; //res < 0 , pipe closed, return 0.
}

int write_nblock(int fd, const void* buf, uint32_t size) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;
	
	if(info.type == FS_TYPE_PIPE) 
		return write_pipe(&info, buf, size);
	if(info.type == FS_TYPE_DIR) 
		return -1;

	mount_t mount;
	if(vfs_get_mount(&info, &mount) != 0)
		return -1;

	int offset = syscall1(SYS_VFS_PROC_TELL, fd);
	if(offset < 0)
		offset = 0;
	
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_WRITE);
	proto_add_int(&in, fd);
	proto_add(&in, &info, sizeof(fsinfo_t));
	proto_add(&in, buf, size);
	proto_add_int(&in, offset);

	int res = -1;
	if(ipc_call(mount.pid, &in, &out) == 0) {
		int r = proto_read_int(&out);
		res = r;
		if(r > 0) {
			offset += r;
			syscall2(SYS_VFS_PROC_SEEK, fd, offset);
		}
		if(res == -2) {
			errno = EAGAIN;
			res = -1;
		}
	}
	proto_clear(&in);
	proto_clear(&out);
	return res;
}

int write(int fd, const void* buf, uint32_t size) {
	while(1) {
		int res = write_nblock(fd, buf, size);
		if(res >= 0)
			return res;

		if(errno != EAGAIN)
			break;
	}
	return -1;
}

void exec_elf(const char* cmd_line, const char* elf, int32_t size) {
	syscall3(SYS_EXEC_ELF, (int32_t)cmd_line, (int32_t)elf, size);
}

int exec(const char* cmd_line) {
	str_t* cmd = str_new("");
	const char *p = cmd_line;
	while(*p != 0 && *p != ' ') {
		str_addc(cmd, *p);
		p++;
	}
	str_addc(cmd, 0);
	int sz;
	void* buf = vfs_readfile(cmd->cstr, &sz);
	if(buf == NULL) {
		str_free(cmd);
		return -1;
	}
	exec_elf(cmd_line, buf, sz);
	free(buf);
	return 0;
}

char* getcwd(char* buf, uint32_t size) {
	syscall2(SYS_PROC_GET_CWD, (int32_t)buf, (int32_t)size);
	return buf;
}

int chdir(const char* path) {
	return syscall1(SYS_PROC_SET_CWD, (int32_t)path);
}

int dup2(int from, int to) {
	return syscall2(SYS_VFS_PROC_DUP2, (int32_t)from, (int32_t)to);
}

int dup(int from) {
	return syscall1(SYS_VFS_PROC_DUP, (int32_t)from);
}

int pipe(int fds[2]) {
	return syscall2(SYS_PIPE_OPEN, (int32_t)&fds[0], (int32_t)&fds[1]);
}
