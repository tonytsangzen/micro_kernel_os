#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cmain.h>
#include <string.h>
#include <ipc.h>
#include <vfs.h>
#include <vdevice.h>
#include <syscall.h>
#include <sd.h>
#include <ext2fs.h>
#include <dev/device.h>
#include <stdio.h>

static void add_file(fsinfo_t* node_to, const char* name, INODE* inode, int32_t ino) {
	fsinfo_t f;
	memset(&f, 0, sizeof(fsinfo_t));
	strcpy(f.name, name);
	f.type = FS_TYPE_FILE;
	f.size = inode->i_size;
	f.data = (uint32_t)ino;

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

static int32_t add_nodes(ext2_t* ext2, INODE *ip, fsinfo_t* dinfo) {
	int32_t i; 
	char c, *cp;
	DIR  *dp;
	char buf[BLOCK_SIZE];

	for (i=0; i<12; i++){
		if (ip->i_block[i] ){
			ext2->read_block(ip->i_block[i], buf);
			dp = (DIR *)buf;
			cp = buf;

			if(dp->inode == 0)
				continue;

			while (cp < &buf[BLOCK_SIZE]){
				c = dp->name[dp->name_len];  // save last byte
				dp->name[dp->name_len] = 0;   
				if(strcmp(dp->name, ".") != 0 && strcmp(dp->name, "..") != 0) {
					int32_t ino = dp->inode;
					INODE ip_node;
					if(ext2_node_by_ino(ext2, ino, &ip_node) == 0) {
						if(dp->file_type == 2) {//director
							fsinfo_t ret;
							add_dir(dinfo, &ret, dp->name);
							add_nodes(ext2, &ip_node, &ret);
						}
						else if(dp->file_type == 1) {//file
							add_file(dinfo, dp->name, &ip_node, ino);
						}
					}
				}
				//add node
				dp->name[dp->name_len] = c; // restore that last byte
				cp += dp->rec_len;
				dp = (DIR *)cp;
			}
		}
	}
	return 0;
}


static int mount(fsinfo_t* mnt_point, mount_info_t* mnt_info, ext2_t* ext2) {
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	strcpy(info.name, mnt_point->name);
	info.type = FS_TYPE_DIR;
	vfs_new_node(&info);

	INODE root_node;
	ext2_node_by_fname(ext2, "/", &root_node);
	add_nodes(ext2, &root_node, &info);

	if(vfs_mount(mnt_point, &info, mnt_info) != 0) {
		vfs_del(&info);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

static int sdext2_mount(fsinfo_t* info, mount_info_t* mnt_info, void* p) {
	mount(info, mnt_info, (ext2_t*)p);
	return 0;
}


static int sdext2_read(int fd, int from_pid, fsinfo_t* info, void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;

	ext2_t* ext2 = (ext2_t*)p;
	int32_t ino = (int32_t)info->data;
	INODE inode;
	if(ext2_node_by_ino(ext2, ino, &inode) != 0) {
		return -1;
	}

	int rsize = info->size - offset;
	if(rsize < size)
		size = rsize;
	if(size < 0)
		size = -1;

	if(size > 0) 
		size = ext2_read(ext2, &inode, buf, size, offset);
	return size;	
}

static int sdext2_umount(fsinfo_t* info, void* p) {
	(void)p;
	vfs_umount(info);
	return 0;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "sd(ext2)");
	dev.mount = sdext2_mount;
	dev.read = sdext2_read;
	dev.umount = sdext2_umount;

	ext2_t ext2;
	ext2_init(&ext2, sd_read, sd_write);

	fsinfo_t root_info;
	vfs_get("/", &root_info);

	fsinfo_t mnt_dir;
	memset(&mnt_dir, 0, sizeof(fsinfo_t));
	strcpy(mnt_dir.name, "mnt");
	mnt_dir.type = FS_TYPE_DIR;
	vfs_new_node(&mnt_dir);
	vfs_add(&root_info, &mnt_dir);

	fsinfo_t mnt_point;
	memset(&mnt_point, 0, sizeof(fsinfo_t));
	strcpy(mnt_point.name, "sd0");
	mnt_point.type = FS_TYPE_DIR;
	vfs_new_node(&mnt_point);
	vfs_add(&mnt_dir, &mnt_point);

	mount_info_t mnt_info;
	strcpy(mnt_info.dev_name, dev.name);
	mnt_info.dev_index = 0;
	mnt_info.access = 0;

	device_run(&dev, &mnt_point, &mnt_info, &ext2, 1);
	ext2_quit(&ext2);
	return 0;
}
