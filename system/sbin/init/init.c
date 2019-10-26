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
			uint32_t sz;
			char* p = ipc_get_msg(&pid, &sz, 1);
			if(p != NULL) {
				debug("msg from: %d: (%d)%s\n", pid, sz, p);
				free(p);
			}
			exit(0);
		}
		else {
			ipc_send_msg(pid, "hello", 6);
			waitpid(pid);
			debug("father waited: c = %d\n", pid);
			//sleep(0);
		}
	}	
	return 0;
}
