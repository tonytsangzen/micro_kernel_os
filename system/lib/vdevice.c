#include <vdevice.h>
#include <vfs.h>
#include <ipc.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <svc_call.h>

static void do_open(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	fsinfo_t info;
	int oflag;
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));
	oflag = proto_read_int(in);

	int32_t fd = -1;
	if(dev != NULL && dev->open != NULL) {
		if(dev->open(&info, oflag, p) == 0)
			fd = svc_call3(SYS_VFS_OPEN, from_pid, (int32_t)&info, oflag);
	}
	else 
			fd = svc_call3(SYS_VFS_OPEN, from_pid, (int32_t)&info, oflag);

	proto_t out;
	proto_init(&out, NULL, 0);
	proto_add_int(&out, fd);
	ipc_send(from_pid, &out);
	proto_clear(&out);
}

static void do_close(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	(void)from_pid;
	fsinfo_t info;
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));

	if(dev != NULL && dev->close != NULL) {
		dev->close(&info, p);
	}
}

static void do_read(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	int size, offset;
	fsinfo_t info;
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));
	size = proto_read_int(in);
	offset = proto_read_int(in);

	proto_t out;
	proto_init(&out, NULL, 0);

	if(dev != NULL && dev->read != NULL) {
		void* buf = malloc(size);
		size = dev->read(&info, buf, size, offset, p);
		proto_add_int(&out, size);
		if(size > 0)
			proto_add(&out, buf, size);
		free(buf);
	}
	else {
		proto_add_int(&out, -1);
	}
	ipc_send(from_pid, &out);
	proto_clear(&out);
}

static void do_write(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	int32_t size, offset;
	fsinfo_t info;
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));
	void* data = proto_read(in, &size);
	offset = proto_read_int(in);

	proto_t out;
	proto_init(&out, NULL, 0);

	if(dev != NULL && dev->write != NULL) {
		size = dev->write(&info, data, size, offset, p);
		proto_add_int(&out, size);
	}
	else {
		proto_add_int(&out, -1);
	}
	ipc_send(from_pid, &out);
	proto_clear(&out);
}

static void do_dma(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	fsinfo_t info;
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));

	proto_t out;
	proto_init(&out, NULL, 0);

	int id = -1;	
	int size = 0;
	if(dev != NULL && dev->dma != NULL) {
		id = dev->dma(&info, &size, p);
	}
	proto_add_int(&out, id);
	proto_add_int(&out, size);
	ipc_send(from_pid, &out);
	proto_clear(&out);
}

static void do_flush(vdevice_t* dev, int from_pid, proto_t *in, void* p) {
	fsinfo_t info;
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));

	if(dev != NULL && dev->flush != NULL) {
		dev->flush(&info, p);
	}

	proto_t out;
	proto_init(&out, NULL, 0);
	proto_add_int(&out, 0);
	ipc_send(from_pid, &out);
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
	}
}

int device_run(vdevice_t* dev, fsinfo_t* mount_point, mount_info_t* mnt_info, void* p) {
	if(dev == NULL)
		return -1;

	if(dev->mount(mount_point, mnt_info, p) != 0)
		return -1;

	proto_t pkg;
	proto_init(&pkg, NULL, 0);
	while(1) {
		int pid;
		if(ipc_recv(&pid, &pkg) == 0) {
			handle(dev, pid, &pkg, p);
			proto_clear(&pkg);
		}
	}

	return dev->umount(mount_point, p);
}
