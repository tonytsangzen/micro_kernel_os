#ifndef FS_INFO_H
#define FS_INFO_H

#include <stdint.h>

#define FS_MOUNT_MAX 32

#define FS_NODE_NAME_MAX 128
#define FS_FULL_NAME_MAX 1024

enum {
	FS_TYPE_DIR = 0,
	FS_TYPE_FILE,
	FS_TYPE_PIPE,
	FS_TYPE_DEV
};

typedef struct {
	uint32_t access;
	char dev_name[FS_NODE_NAME_MAX];
	uint32_t dev_index;
} mount_info_t;

typedef struct {
	int32_t pid;
	uint32_t org_node;
	char org_name[FS_NODE_NAME_MAX];

	mount_info_t info;
} mount_t;

typedef struct {
	uint32_t node;
	char name[FS_NODE_NAME_MAX];
	uint32_t type;
	uint32_t size;
	uint32_t owner;
	int32_t mount_id;

	uint32_t data;
} fsinfo_t;

#endif
