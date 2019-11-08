#include <stddef.h>
#include <unistd.h>
#include <svc_call.h>
#include <proto.h>
#include <vfs.h>
#include <ipc.h>
#include <string.h>

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
