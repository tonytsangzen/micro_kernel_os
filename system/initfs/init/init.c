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
		//debug("iii\n");
		sleep(0);
		ramfs_t ramfs;
		const char* initrd = (const char*)svc_call0(SYS_INITRD);

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

	fsinfo_t info;
	vfs_get("/initfs/init", &info);

	int fd, seek;
	fd = vfs_open(getpid(), &info, 1);
	debug("fd: %d\n", fd);

	seek = vfs_seek(fd, 100, 0);
	debug("seek: %d\n", seek);

	vfs_get_by_fd(fd, &info);
	debug("name: %s\n", info.name);

	vfs_close(getpid(), fd);

	return 0;
}
