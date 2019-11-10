#include <unistd.h>
#include <stdio.h>
#include <vfs.h>
#include <vprintf.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	const char* r;
	char cwd[FS_FULL_NAME_MAX];

	if(argc >= 2)
		r = argv[1];
	else 
		r = getcwd(cwd, FS_FULL_NAME_MAX);

	fsinfo_t info;
	if(vfs_get(r, &info) != 0)
		return -1;
	if(vfs_first_kid(&info, &info) != 0)
		return -1;

	printf("  NAME                     TYPE  OWNER  SIZE\n");
	while(1) {
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
