#include <svc_call.h>
#include <stdlib.h>
#include <unistd.h>
#include <vprintf.h>
#include <sys/wait.h>
#include <uart_debug.h>

void _start(void) {
	char s[32];
	while(1) {
		int pid = fork();
		if(pid == 0) {
			uart_debug("child\n");
			exit(0);
		}
		else {
			waitpid(pid);
			snprintf(s, 31, "father waited: c = %d\n", pid);
			uart_debug(s);
			//sleep(0);
		}
	}	
}
