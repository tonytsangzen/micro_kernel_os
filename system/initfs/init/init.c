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

	int pid = fork();
	if(pid == 0) {
		const char* initrd = (const char*)svc_call0(SYS_INITRD);
		ramfs_t ramfs;

		ramfs_open(initrd, &ramfs);
		const char* elf = ramfs_read(&ramfs, "initfsd", NULL);

		svc_call1(SYS_EXEC_ELF, (int32_t)elf);
		ramfs_close(&ramfs);
	}

	while(1) {
		void* p = ipc_get_msg(NULL, NULL, 1);
		if(p != NULL) {
			free(p);
			break;
		}
	}

	fsinfo_t root, info;
	mount_t mount;
	vfs_get("/", &root);
	vfs_first_kid(&root, &info);
	vfs_get_mount(&info, &mount);
	debug("%d, %s\n", mount.pid, info.name);
	vfs_next(&info, &info);
	vfs_get_mount(&info, &mount);
	debug("%d, %s\n", mount.pid, info.name);

	while(1) {
		sleep(0);		
	}
}
