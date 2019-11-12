#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int i = 0;
	while(i < 1000) {
		int fds[2];
		pipe(fds);

		char buf[32];
		snprintf(buf, 32, "hello %d!", i++);

		write(fds[0], buf, strlen(buf)+1);
		buf[0] = 0;
		read(fds[1], buf, 32);
		printf("%d, %d: %s\n", fds[0], fds[1], buf);

		close(fds[0]);
		close(fds[1]);
	}
	return 0;
}
