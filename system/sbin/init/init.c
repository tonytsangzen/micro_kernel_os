#include <svc_call.h>
#include <stdlib.h>
#include <unistd.h>
#include <vprintf.h>
#include <sys/wait.h>
#include <debug.h>
#include <ipc.h>
#include <cmain.h>

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	while(1) {
		int pid = fork();
		if(pid == 0) {
			sleep(0);
			debug("child\n");
			char* p = ipc_get_msg(NULL, NULL);
			if(p != NULL) {
				debug(p);
				free(p);
			}
			exit(0);
		}
		else {
			ipc_send_msg(pid, "hello\n", 7);
			waitpid(pid);
			debug("father waited: c = %d\n", pid);
			//sleep(0);
		}
	}	
	return 0;
}
