#include <stdlib.h>
#include <unistd.h>
#include <dev/devicetype.h>
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

	char s[8];
	while(1) {
		memset(s, 0, 8);
		int i = svc_call3(SYS_DEV_READ, DEV_UART0, (int32_t)s, 8);
		if(i != 0)
			debug("%s", s);
	}

	int pid = fork();
	if(pid == 0) {
		ramfs_t ramfs;
		const char* initrd = (const char*)svc_call0(SYS_INITRD);

		ramfs_open(initrd, &ramfs);
		const char* elf = ramfs_read(&ramfs, "initfsd", NULL);

		svc_call1(SYS_EXEC_ELF, (int32_t)elf);
		ramfs_close(&ramfs);
		exit(0);
	}

	while(1) {
		fsinfo_t info;
		debug("1\n");
		if(vfs_get("/initfs/init", &info) == 0)
			break;
		debug("2\n");
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
