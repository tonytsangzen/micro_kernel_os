#ifndef FS_INFO_H
#define FS_INFO_H

#include <stdint.h>

#define FS_NODE_NAME_MAX 128
#define FS_FULL_NAME_MAX 1024

#define FS_TYPE_DIR   0
#define FS_TYPE_FILE  1 

typedef struct {
	uint32_t node;
	char name[FS_NODE_NAME_MAX];
	uint32_t type;
	int32_t mount_pid;
} fsinfo_t;

#endif
