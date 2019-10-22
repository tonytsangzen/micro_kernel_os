#include <svc_call.h>
#include <stdlib.h>
#include <unistd.h>
#include <vprintf.h>
#include <uart_debug.h>

void _start(void) {
	int pid = fork();
	char s[32];
	if(pid == 0) {
		snprintf(s, 31, "new proc\n");
		uart_debug(s);
	}
	else {
		snprintf(s, 31, "child pid %d\n", pid);
		uart_debug(s);
	}
	exit(0);
}
