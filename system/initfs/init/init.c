#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dev/device.h>
#include <dev/fbinfo.h>
#include <sys/wait.h>
#include <debug.h>
#include <cmain.h>
#include <string.h>
#include <fcntl.h>
#include <vfs.h>
#include <svc_call.h>

static void init_stdio(void) {
	int fd = open("/dev/tty0", 0);
	dup2(fd, 0);
	dup2(fd, 1);
}

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
		exec("/sbin/nulld");
	}
	vfs_mount_wait("/dev/null", pid);
	debug("/dev/null mounted.\n");

	pid = fork();
	if(pid == 0) {
		exec("/sbin/ttyd");
	}
	vfs_mount_wait("/dev/tty0", pid);
	debug("/dev/tty0 mounted.\n\n");

	pid = fork();
	if(pid == 0) {
		init_stdio();
		exec("/sbin/shell");
	}
	
	while(1) {
	uint32_t sz = 1024*768*4;
	void* buf = malloc(sz);
	memset(buf, 0xff, sz);
	svc_call3(SYS_DEV_WRITE, DEV_FRAMEBUFFER, (int32_t)buf, sz);
	free(buf);

		sleep(0);
	}
	return 0;
}
