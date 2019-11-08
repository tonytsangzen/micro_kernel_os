#include <stdlib.h>
#include <unistd.h>
#include <dev/kdevicetype.h>
#include <sys/wait.h>
#include <debug.h>
#include <cmain.h>
#include <string.h>
#include <fcntl.h>
#include <vfs.h>
#include <ramfs.h>
#include <svc_call.h>

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	int pid = fork();
	if(pid == 0) {
		ramfs_t ramfs;
		const char* initrd = (const char*)svc_call0(SYS_INITRD);

		ramfs_open(initrd, &ramfs);
		int sz;
		const char* elf = ramfs_read(&ramfs, "initfsd", &sz);

		svc_call2(SYS_EXEC_ELF, (int32_t)elf, sz);
	}

	while(1) {
		fsinfo_t info;
		if(vfs_get("/initfs/ttyd", &info) == 0)
			break;
	}

	pid = fork();
	if(pid == 0) {
		exec("/initfs/ttyd");
	}

	while(1) {
		fsinfo_t info;
		if(vfs_get("/dev/tty0", &info) == 0)
			break;
	}

	pid = fork();
	if(pid == 0) {
		exec("/initfs/shell");
	}

/*	int fd = open("/initfs/Makefile", 0);
	debug("open fd: %d\n", fd);

	char buf[128];
	while(1) {
		int sz = read(fd, buf, 127);
		if(sz <= 0)
			break;
		buf[sz] = 0;
		debug("%s", buf);
	}

	close(fd);
	debug("closed fd: %d\n", fd);
	*/
	while(1) {
		sleep(0);
	}	
	return 0;
}
