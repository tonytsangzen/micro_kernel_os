#include <svc_call.h>
#include <stdlib.h>
#include <unistd.h>
#include <vprintf.h>
#include <uart_debug.h>

void _start(void) {
	while(1) {
		int pid = fork();
		char s[32];
		if(pid == 0) {
			snprintf(s, 31, "child: %d\n", getpid());
			uart_debug(s);
			exit(0);
		}
		else {
			snprintf(s, 31, "create pid : %d\n", pid);
			uart_debug(s);
		}
	}
}
