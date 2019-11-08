#include <stdlib.h>
#include <unistd.h>
#include <dev/kdevicetype.h>
#include <sys/wait.h>
#include <debug.h>
#include <cmain.h>
#include <string.h>
#include <fcntl.h>
#include <vfs.h>
#include <svc_call.h>

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	debug("\n---mkos starting---\n");

	int pid = fork();
	if(pid == 0) {
		exec_initfs("initfsd");
	}
	vfs_mount_wait("/sbin", pid);
	debug("/sbin mounted.\n");

	pid = fork();
	if(pid == 0) {
		exec("/sbin/ttyd");
	}
	vfs_mount_wait("/dev/tty0", pid);
	debug("/dev/tty0 mounted.\n\n");

	pid = fork();
	if(pid == 0) {
		exec("/sbin/shell");
	}

	while(1) {
		sleep(0);
	}
	return 0;
}
