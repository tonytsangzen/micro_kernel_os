#include <unistd.h>
#include <stdio.h>
#include <vfs.h>
#include <vprintf.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	char cwd[FS_FULL_NAME_MAX];
	getcwd(cwd, FS_FULL_NAME_MAX);

	
	fsinfo_t info;
	if(vfs_get(cwd, &info) != 0)
		return -1;
	if(vfs_first_kid(&info, &info) != 0)
		return -1;

	char full[FS_FULL_NAME_MAX];
	printf("  NAME                     TYPE  OWNER  SIZE\n");
	while(1) {
		if(strcmp(cwd, "/") != 0)
			snprintf(full, FS_FULL_NAME_MAX-1, "%s/%s", cwd, info.name);
		else
			snprintf(full, FS_FULL_NAME_MAX-1, "/%s", info.name);

		if(info.type == FS_TYPE_FILE)
			printf("  %24s  f    %4d   %dK\n", info.name, info.owner, info.size/1024);
		else if(info.type == FS_TYPE_DIR)
			printf("  %24s  r    %4d   %dK\n", info.name, info.owner, info.size/1024);
		else //if(info.type == FS_TYPE_DEV)
			printf("  %24s  d    %4d   %dK\n", info.name, info.owner, info.size/1024);

		if(vfs_next(&info, &info) != 0)
			break;
	}
	return 0;
}
