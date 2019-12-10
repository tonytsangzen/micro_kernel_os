#include <vdevice.h>
#include <vfs.h>
#include <ipc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syscall.h>

static void do_open(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	fsinfo_t info;
	int oflag;
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));
	oflag = proto_read_int(in);

	int32_t fd = syscall3(SYS_VFS_OPEN, from_pid, (int32_t)&info, oflag);
	if(fd >= 0 && dev != NULL && dev->open != NULL) {
		if(dev->open(fd, from_pid, &info, oflag, p) != 0) {
		}
	}

	proto_t out;
	proto_init(&out, NULL, 0);
	proto_add_int(&out, fd);
	ipc_send(from_pid, &out, in->id);
	proto_clear(&out);
}

static void do_close(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	(void)from_pid;
	fsinfo_t info;
	int fd = proto_read_int(in);
	proto_read_to(in, &info, sizeof(fsinfo_t));

	if(dev != NULL && dev->close != NULL) {
		dev->close(fd, from_pid, &info, p);
	}
}

static void do_closed(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	(void)from_pid;
	fsinfo_t info;
	int fd = proto_read_int(in);
	proto_read_to(in, &info, sizeof(fsinfo_t));
	if(dev != NULL && dev->closed != NULL) {
		dev->closed(fd, from_pid, &info, p);
	}
}

static void do_read(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	int size, offset;
	fsinfo_t info;
	int fd = proto_read_int(in);
	proto_read_to(in, &info, sizeof(fsinfo_t));
	size = proto_read_int(in);
	offset = proto_read_int(in);

	proto_t out;
	proto_init(&out, NULL, 0);

	if(dev != NULL && dev->read != NULL) {
		void* buf = malloc(size);
		size = dev->read(fd, from_pid, &info, buf, size, offset, p);
		proto_add_int(&out, size);
		if(size > 0)
			proto_add(&out, buf, size);
		free(buf);
	}
	else {
		proto_add_int(&out, -1);
	}
	ipc_send(from_pid, &out, in->id);
	proto_clear(&out);
}

static void do_write(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	int32_t size, offset;
	fsinfo_t info;
	int fd = proto_read_int(in);
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));
	void* data = proto_read(in, &size);
	offset = proto_read_int(in);

	proto_t out;
	proto_init(&out, NULL, 0);

	if(dev != NULL && dev->write != NULL) {
		size = dev->write(fd, from_pid, &info, data, size, offset, p);
		proto_add_int(&out, size);
	}
	else {
		proto_add_int(&out, -1);
	}
	ipc_send(from_pid, &out, in->id);
	proto_clear(&out);
}

static void do_dma(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	fsinfo_t info;
	int fd = proto_read_int(in);
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));

	proto_t out;
	proto_init(&out, NULL, 0);

	int id = -1;	
	int size = 0;
	if(dev != NULL && dev->dma != NULL) {
		id = dev->dma(fd, from_pid, &info, &size, p);
	}
	proto_add_int(&out, id);
	proto_add_int(&out, size);
	ipc_send(from_pid, &out, in->id);
	proto_clear(&out);
}

static void do_cntl(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	fsinfo_t info;
	int fd = proto_read_int(in);
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));
	int32_t cmd = proto_read_int(in);

	proto_t arg_in, arg_out, out;
	proto_init(&arg_out, NULL, 0);
	proto_init(&out, NULL, 0);

	int32_t arg_size;
	void* arg_data = proto_read(in, &arg_size);
	proto_init(&arg_in, arg_data, arg_size);

	int res = -1;
	if(dev != NULL && dev->cntl != NULL) {
		res = dev->cntl(fd, from_pid, &info, cmd, &arg_in, &arg_out, p);
	}
	proto_clear(&arg_in);

	proto_add_int(&out, res);
	arg_data = proto_read(&arg_out, &arg_size);
	proto_add(&out, arg_data, arg_size);
	proto_clear(&arg_out);

	ipc_send(from_pid, &out, in->id);
	proto_clear(&out);
}

static void do_flush(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	fsinfo_t info;
	int fd = proto_read_int(in);
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));

	if(dev != NULL && dev->flush != NULL) {
		dev->flush(fd, from_pid, &info, p);
	}

	proto_t out;
	proto_init(&out, NULL, 0);
	proto_add_int(&out, 0);
	ipc_send(from_pid, &out, in->id);
	proto_clear(&out);
}

static void do_create(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	fsinfo_t info_to, info;
	proto_read_to(in, &info_to, sizeof(fsinfo_t));
	proto_read_to(in, &info, sizeof(fsinfo_t));

	int res = 0;
	if(dev != NULL && dev->create != NULL) {
		res = dev->create(&info_to, &info, p);
	}

	proto_t out;
	proto_init(&out, NULL, 0);
	proto_add_int(&out, res);
	proto_add(&out, &info, sizeof(fsinfo_t));
	ipc_send(from_pid, &out, in->id);
	proto_clear(&out);
}

static void do_unlink(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	fsinfo_t info_to, info;
	proto_read_to(in, &info_to, sizeof(fsinfo_t));
	const char* fname = proto_read_str(in);

	int res = 0;
	if(dev != NULL && dev->unlink != NULL) {
		res = dev->unlink(&info, fname, p);
	}

	proto_t out;
	proto_init(&out, NULL, 0);
	proto_add_int(&out, res);
	ipc_send(from_pid, &out, in->id);
	proto_clear(&out);
}

static void do_block(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	fsinfo_t info_to, info;
	proto_read_to(in, &info_to, sizeof(fsinfo_t));

	int res = -1;
	if(dev != NULL && dev->block != NULL) {
		res = dev->block(&info, p);
	}

	proto_t out;
	proto_init(&out, NULL, 0);
	proto_add_int(&out, res);
	ipc_send(from_pid, &out, in->id);
	proto_clear(&out);
}

static void handle(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	if(dev == NULL)
		return;

	int cmd = proto_read_int(in);

	switch(cmd) {
	case FS_CMD_OPEN:
		do_open(dev, from_pid, in, p);
		return;
	case FS_CMD_CLOSE:
		do_close(dev, from_pid, in, p);
		return;
	case FS_CMD_CLOSED:
		do_closed(dev, from_pid, in, p);
		return;
	case FS_CMD_READ:
		do_read(dev, from_pid, in, p);
		return;
	case FS_CMD_WRITE:
		do_write(dev, from_pid, in, p);
		return;
	case FS_CMD_DMA:
		do_dma(dev, from_pid, in, p);
		return;
	case FS_CMD_FLUSH:
		do_flush(dev, from_pid, in, p);
		return;
	case FS_CMD_CNTL:
		do_cntl(dev, from_pid, in, p);
		return;
	case FS_CMD_CREATE:
		do_create(dev, from_pid, in, p);
		return;
	case FS_CMD_UNLINK:
		do_unlink(dev, from_pid, in, p);
		return;
	case FS_CMD_BLOCK:
		do_block(dev, from_pid, in, p);
		return;
	}
}

int device_run(vdevice_t* dev, fsinfo_t* mount_point, mount_info_t* mnt_info, void* p, int block) {
	if(dev == NULL)
		return -1;

	if(dev->mount(mount_point, mnt_info, p) != 0)
		return -1;

	proto_t pkg;
	proto_init(&pkg, NULL, 0);
	while(1) {
		int pid;
		if(ipc_get(&pid, &pkg, -1, block) >= 0) {
			handle(dev, pid, &pkg, p);
			proto_clear(&pkg);
		}
		if(dev->loop_step != NULL)
			dev->loop_step(p);
		if(block == 0)
			sleep(0);
	}

	return dev->umount(mount_point, p);
}
