#include <svc_call.h>
#include <stdlib.h>
#include <string.h>
#include <uart_debug.h>

void _start(void) {
	char* s = malloc(100);
	strcpy(s, "Hello, World!\n");
	uart_debug(s);
	free(s);
	exit(0);
}
