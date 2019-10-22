#include <svc_call.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <uart_debug.h>

void _start(void) {
	int pid = fork();

	if(pid == 0) {
		char* s = malloc(100);
		strcpy(s, "Hello, World child!\n");
		uart_debug(s);
		free(s);
		exit(0);
	}
	else {
		char* s = malloc(100);
		strcpy(s, "Hello, World parent!\n");
		uart_debug(s);
		free(s);
		exit(0);
	}
}
