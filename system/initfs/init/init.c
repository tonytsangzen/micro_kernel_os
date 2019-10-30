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

	const char* initrd = (const char*)svc_call0(SYS_INITRD);
	ramfs_t ramfs;

	ramfs_open(initrd, &ramfs);
	const char* elf = ramfs_read(&ramfs, "initfsd", NULL);

	int pid = fork();
	if(pid == 0) {
		debug("load elf\n");
		svc_call1(SYS_LOAD_ELF, (int32_t)elf);
		debug("load elf end\n");
		exit(0);
	}

	ramfs_close(&ramfs);
	while(1) {
		sleep(0);		
	}
}
