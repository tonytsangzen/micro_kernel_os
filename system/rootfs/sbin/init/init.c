#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cmain.h>
#include <string.h>
#include <fcntl.h>
#include <vfs.h>

static void init_stdio(void) {
	int fd = open("/dev/tty0", 0);
	dup2(fd, 0);
	dup2(fd, 1);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	uprintf("\n------ mkos -------\n");
	int pid = fork();
	if(pid == 0) {
		exec_initfs("sbin/dev/initrd");
	}
	vfs_mount_wait("/dev", pid);
	uprintf("rootfs(initrd) mounted to /.\n");

	setenv("OS", "mkos");
	setenv("PATH", "/sbin:/bin");

	pid = fork();
	if(pid == 0) {
		exec("/sbin/dev/nulld");
	}
	vfs_mount_wait("/dev/null", pid);
	uprintf("device null mounted to /dev/null.\n");

	pid = fork();
	if(pid == 0) {
		exec("/sbin/dev/ttyd");
	}
	vfs_mount_wait("/dev/tty0", pid);
	uprintf("device uart mounted to /dev/tty0.\n");

	pid = fork();
	if(pid == 0) {
		exec("/sbin/dev/fbd");
	}
	vfs_mount_wait("/dev/fb0", pid);
	uprintf("device framebuffer mounted to /dev/fb0.\n");

	pid = fork();
	if(pid == 0) {
		exec("/sbin/dev/keybd");
	}
	vfs_mount_wait("/dev/keyb0", pid);
	uprintf("device keyboard mounted to /dev/keyb0.\n");

	pid = fork();
	if(pid == 0) {
		exec("/sbin/dev/moused");
	}
	vfs_mount_wait("/dev/mouse0", pid);
	uprintf("device mouse mounted to /dev/mouse0.\n");

	pid = fork();
	if(pid == 0) {
		exec("/sbin/dev/xserverd");
	}
	vfs_mount_wait("/dev/x", pid);
	uprintf("device xserver mounted to /dev/x.\n\n");

	pid = fork();
	if(pid == 0) {
		init_stdio();
		exec("/bin/xconsole");
	}

	pid = fork();
	if(pid == 0) {
		init_stdio();
		exec("/bin/shell");
	}

	while(1) {
		sleep(0);
	}
	return 0;
}
