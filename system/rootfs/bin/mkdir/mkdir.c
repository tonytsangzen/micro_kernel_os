#include <unistd.h>
#include <vfs.h>
#include <stdio.h>
#include <vprintf.h>
#include <cmain.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: mkdir <dir>");
		return -1;
	}

	char dn[FS_FULL_NAME_MAX];
	char cwd[FS_FULL_NAME_MAX];
	snprintf(dn, FS_FULL_NAME_MAX-1, "%s/%s", getcwd(cwd, FS_FULL_NAME_MAX-1), argv[1]);
	
	fsinfo_t info;
	return vfs_create(dn, &info, FS_TYPE_DIR);
}
