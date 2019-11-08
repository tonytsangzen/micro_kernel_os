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

	int fd = open("/dev/tty0", 0);

	char s[8];
	while(1) {
		memset(s, 0, 8);
		int i = read(fd, s, 7);
		if(i != 0)
			write(fd, s, i);
	}

	close(fd);
	
	return 0;
}
