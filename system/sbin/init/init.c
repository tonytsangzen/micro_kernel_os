#include <svc_call.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <uart_debug.h>

void _start(void) {
	int pid = fork();

	if(pid == 0) {
		uart_debug("child1\n");
		uart_debug("child2\n");
		exit(0);
	}

	uart_debug("father\n");
	while(1);
}
