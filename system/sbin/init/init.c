#include <svc_call.h>
#include <stdlib.h>
#include <unistd.h>
#include <vprintf.h>
#include <sys/wait.h>
#include <debug.h>
#include <ipc.h>
#include <cmain.h>
#include <fsinfo.h>
#include <string.h>


int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	fsinfo_t info;
	svc_call1(SYS_VFS_NEW_NODE, (int32_t)&info);
	info.type = FS_TYPE_FILE;
	strcpy(info.name, "sbin");
	svc_call1(SYS_VFS_SET_INFO, (int32_t)&info);

	fsinfo_t root_info;

	svc_call2(SYS_VFS_GET_INFO, (int32_t)"/", (int32_t)&root_info);
	svc_call3(SYS_VFS_MOUNT, (int32_t)&root_info, (int32_t)&info, 0);
	svc_call2(SYS_VFS_GET_INFO, (int32_t)"/", (int32_t)&root_info);
	svc_call1(SYS_VFS_UMOUNT, (int32_t)&root_info);


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
