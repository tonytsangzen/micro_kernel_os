#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <debug.h>
#include <cmain.h>
#include <string.h>
#include <ipc.h>
#include <vfs.h>
#include <ramfs.h>
#include <svc_call.h>

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	debug("initfs loading......\n");

	fsinfo_t initfs;
	vfs_new_node("initfs", FS_TYPE_DIR, &initfs);

	fsinfo_t root_info;
	vfs_get("/", &root_info);
	vfs_add(&root_info, &initfs);

	ramfs_t ramfs;
	const char* initrd = (const char*)svc_call0(SYS_INITRD);
	ramfs_open(initrd, &ramfs);

	debug("initfs loaded\n");

	fsinfo_t info;
	vfs_new_node("initfs", FS_TYPE_DIR, &info);

	ram_file_t* rf = ramfs.head;
	while(rf != NULL) {
		fsinfo_t f;
		vfs_new_node(rf->name, FS_TYPE_FILE, &f);
		vfs_add(&info, &f);
		rf = rf->next;
	}

	vfs_mount(&initfs, &info, 0);
	
	proto_t *pkg = proto_new(NULL, 0);
	proto_add_int(pkg, 1024);
	ipc_send_pkg(0, pkg);

	while(1) {
		//sleep(0);
	}

	vfs_umount(&info);
	ramfs_close(&ramfs);
	return 0;
}
