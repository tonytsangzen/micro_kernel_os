#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cmain.h>
#include <string.h>
#include <ipc.h>
#include <vfs.h>
#include <vdevice.h>
#include <ramfs.h>
#include <svc_call.h>

/*
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

static void add_dir(fsinfo_t* node, const char* name) {
	fsinfo_t f;
	if(vfs_get(node, name, )

	memset(&f, 0, sizeof(fsinfo_t));
	strcpy(f.name, name);
	f.type = FS_TYPE_DIR;
	vfs_new_node(&f);
	vfs_add(node_to, &f);
	memcpy(node_to, &f, sizeof(fsinfo_t);
}

static void add_node(fsinfo_t* node_to, ram_file_t* rf) {
	const char* name = rf->name;
	char n[FS_FULL_NAME_MAX+1];
	int32_t j = 0;
	for(int32_t i=0; i<FS_FULL_NAME_MAX; i++) {
		n[i] = name[i];
		if(n[i] == 0) {
			return vfs_simple_get(node, n+j);
		}
		if(n[i] == '/') {
			n[i] = 0; 
			node = vfs_simple_get(node, n+j);
			if(node == NULL)
				return NULL;
			j= i+1;
		}
	}

	
	fsinfo_t f;
	memset(&f, 0, sizeof(fsinfo_t));
	f.type = FS_TYPE_FILE;
	f.size = rf->size;
	f.data = (uint32_t)rf->content;

	vfs_new_node(&f);
	vfs_add(&info, &f);
	rf = rf->next;
}
*/

static int mount(fsinfo_t* mnt_point, mount_info_t* mnt_info, ramfs_t* ramfs) {
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	strcpy(info.name, mnt_point->name);
	info.type = FS_TYPE_DIR;
	vfs_new_node(&info);

	ram_file_t* rf = ramfs->head;
	while(rf != NULL) {
		fsinfo_t f;
		memset(&f, 0, sizeof(fsinfo_t));
		strcpy(f.name, rf->name);
		f.type = FS_TYPE_FILE;
		f.size = rf->size;
		f.data = (uint32_t)rf->content;

		vfs_new_node(&f);
		vfs_add(&info, &f);
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

static int initfs_read(fsinfo_t* info, void* buf, int size, int offset, void* p) {
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
	const char* initrd = (const char*)svc_call0(SYS_INITRD);
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
