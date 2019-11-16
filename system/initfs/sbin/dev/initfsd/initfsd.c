#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cmain.h>
#include <string.h>
#include <ipc.h>
#include <vfs.h>
#include <vdevice.h>
#include <ramfs.h>
#include <syscall.h>

static void add_file(fsinfo_t* node_to, const char* name, ram_file_t* rf) {
	fsinfo_t f;
	memset(&f, 0, sizeof(fsinfo_t));
	strcpy(f.name, name);
	f.type = FS_TYPE_FILE;
	f.size = rf->size;
	f.data = (uint32_t)rf->content;

	vfs_new_node(&f);
	vfs_add(node_to, &f);
}

static int add_dir(fsinfo_t* node_to, fsinfo_t* ret, const char* dn) {
	memset(ret, 0, sizeof(fsinfo_t));
	strcpy(ret->name, dn);
	ret->type = FS_TYPE_DIR;
	vfs_new_node(ret);
	if(vfs_add(node_to, ret) != 0) {
		vfs_del(ret);
		return -1;
	}
	return 0;
}

static void add_node(fsinfo_t* node_to, ram_file_t* rf) {
	const char* name = rf->name;
	char n[FS_FULL_NAME_MAX+1];
	int32_t j = 0;

	fsinfo_t pnode;
	fsinfo_t cnode;
	memcpy(&pnode, node_to, sizeof(fsinfo_t));

	for(int32_t i=0; i<FS_FULL_NAME_MAX; i++) {
		n[i] = name[i];
		if(n[i] == 0) {
			add_file(&pnode, n+j, rf);
			return;
		}
		if(n[i] == '/') {
			n[i] = 0; 

			add_dir(&pnode, &cnode, n+j);
			memcpy(&pnode, &cnode, sizeof(fsinfo_t));
			j= i+1;
		}
	}
}

static int mount(fsinfo_t* mnt_point, mount_info_t* mnt_info, ramfs_t* ramfs) {
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	strcpy(info.name, mnt_point->name);
	info.type = FS_TYPE_DIR;
	vfs_new_node(&info);

	ram_file_t* rf = ramfs->head;
	while(rf != NULL) {
		add_node(&info, rf);
		rf = rf->next;
	}

	if(vfs_mount(mnt_point, &info, mnt_info) != 0) {
		vfs_del(&info);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

static int initfs_mount(fsinfo_t* info, mount_info_t* mnt_info, void* p) {
	mount(info, mnt_info, (ramfs_t*)p);
	return 0;
}

static int initfs_read(int fd, int from_pid, fsinfo_t* info, void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)p;
	const char* content = (const char*)info->data + offset;
	int rsize = info->size - offset;
	if(rsize < size)
		size = rsize;
	if(size < 0)
		size = -1;

	if(size > 0)
		memcpy(buf, content, size);
	return size;	
}

static int initfs_umount(fsinfo_t* info, void* p) {
	(void)p;
	vfs_umount(info);
	return 0;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "ramfs");
	dev.mount = initfs_mount;
	dev.read = initfs_read;
	dev.umount = initfs_umount;

	ramfs_t ramfs;
	const char* initrd = (const char*)syscall0(SYS_INITRD);
	ramfs_open(initrd, &ramfs);

	fsinfo_t root_info;
	vfs_get("/", &root_info);

	fsinfo_t dev_dir;
	memset(&dev_dir, 0, sizeof(fsinfo_t));
	strcpy(dev_dir.name, "dev");
	dev_dir.type = FS_TYPE_DIR;
	vfs_new_node(&dev_dir);
	vfs_add(&root_info, &dev_dir);

	fsinfo_t mnt_point;
	memset(&mnt_point, 0, sizeof(fsinfo_t));
	strcpy(mnt_point.name, "initrd");
	mnt_point.type = FS_TYPE_DIR;

	vfs_new_node(&mnt_point);
	vfs_add(&root_info, &mnt_point);

	mount_info_t mnt_info;
	strcpy(mnt_info.dev_name, dev.name);
	mnt_info.dev_index = 0;
	mnt_info.access = 0;

	device_run(&dev, &mnt_point, &mnt_info, &ramfs);
	ramfs_close(&ramfs);
	return 0;
}
