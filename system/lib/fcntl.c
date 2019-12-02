#include <fcntl.h>
#include <syscall.h>
#include <ipc.h>
#include <vfs.h>
#include <stddef.h>
#include <string.h>

static int create(const char* fname, fsinfo_t* ret, int type) {
	str_t *dir = str_new("");
	str_t *name = str_new("");
	vfs_parse_name(fname, dir, name);

	fsinfo_t info_to;
	if(vfs_get(CS(dir), &info_to) != 0) {
		str_free(dir);
		str_free(name);
		return -1;
	}
	
	mount_t mount;
	if(vfs_get_mount(&info_to, &mount) != 0) {
		str_free(dir);
		str_free(name);
		return -1;
	}

	memset(ret, 0, sizeof(fsinfo_t));
	strcpy(ret->name, CS(name));
	ret->type = type;
	str_free(name);
	str_free(dir);

	vfs_new_node(ret);
	if(vfs_add(&info_to, ret) != 0) {
		vfs_del(ret);
		return -1;
	}
	
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_CREATE);
	proto_add(&in, &info_to, sizeof(fsinfo_t));
	proto_add(&in, ret, sizeof(fsinfo_t));

	int res = 1;
	if(ipc_call(mount.pid, &in, &out) != 0) {
		vfs_del(ret);
	}
	else {
		proto_read_to(&out, ret, sizeof(fsinfo_t));
		res = 0;
	}
	proto_clear(&in);
	proto_clear(&out);

	return res;
}

int open(const char* fname, int oflag) {
	int res = -1;
	fsinfo_t info;

	if(vfs_get(fname, &info) != 0) {
		if((oflag & O_CREAT) != 0)
			if(create(fname, &info, FS_TYPE_FILE) != 0)
				return -1;
	}
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) != 0)
		return -1;
	
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_OPEN);
	proto_add(&in, &info, sizeof(fsinfo_t));
	proto_add_int(&in, oflag);

	if(ipc_call(mount.pid, &in, &out) == 0) {
		res = proto_read_int(&out);
	}

	proto_clear(&in);
	proto_clear(&out);
	return res;
}

void close(int fd) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return;
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) == 0) {
		proto_t in;
		proto_init(&in, NULL, 0);

		proto_add_int(&in, FS_CMD_CLOSE);
		proto_add_int(&in, fd);
		proto_add(&in, &info, sizeof(fsinfo_t));

		ipc_call(mount.pid, &in, NULL);
		proto_clear(&in);
	}

	syscall1(SYS_VFS_PROC_CLOSE, fd);
}

int dma(int fd, int* size) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) != 0)
		return -1;

	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_DMA);
	proto_add_int(&in, fd);
	proto_add(&in, &info, sizeof(fsinfo_t));

	int shm_id = -1;
	if(ipc_call(mount.pid, &in, &out) == 0) {
		shm_id = proto_read_int(&out);
		if(size != NULL)
			*size = proto_read_int(&out);
	}
	proto_clear(&in);
	proto_clear(&out);
	return shm_id;
}

void flush(int fd) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return;
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) == 0) {
		proto_t in, out;
		proto_init(&in, NULL, 0);
		proto_init(&out, NULL, 0);

		proto_add_int(&in, FS_CMD_FLUSH);
		proto_add_int(&in, fd);
		proto_add(&in, &info, sizeof(fsinfo_t));
		ipc_call(mount.pid, &in, &out);
		proto_clear(&in);
		proto_clear(&out);
	}
}

int cntl_raw(int fd, int cmd, proto_t* arg_in, proto_t* arg_out) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) != 0)
		return -1;

	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_CNTL);
	proto_add_int(&in, fd);
	proto_add(&in, &info, sizeof(fsinfo_t));
	proto_add_int(&in, cmd);
	if(arg_in == NULL)
		proto_add(&in, NULL, 0);
	else
		proto_add(&in, arg_in->data, arg_in->size);

	int res = -1;
	if(ipc_call(mount.pid, &in, &out) == 0) {
		res = proto_read_int(&out);
		if(arg_out != NULL) {
			int32_t sz;
			void *p = proto_read(&out, &sz);
			proto_add(arg_out, p, sz);
		}
	}
	proto_clear(&in);
	proto_clear(&out);
	return res;
}
