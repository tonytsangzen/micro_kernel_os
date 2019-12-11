#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <string.h>
#include <fsinfo.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	printf("  MOUNT_POINT              DRIVER           DEV_ID  PID\n");
	int32_t i;
	for(i=0; i<FS_MOUNT_MAX; i++) {
		mount_t mnt;
		if(syscall2(SYS_VFS_GET_MOUNT_BY_ID, i, (int32_t)&mnt) != 0)
			continue;
		printf("  %24s %16s %6d  %6d\n", 
				mnt.org_name,
				mnt.info.dev_name,
				mnt.info.dev_index,
				mnt.pid);
	}
	return 0;
}
