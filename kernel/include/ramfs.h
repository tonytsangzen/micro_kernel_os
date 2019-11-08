#ifndef RAMFS_H
#define RAMFS_H 

/*
very simple read only RAM file system for kernel.
	fs: [file0][file1][file...]
	file: [name_len: 4][name: name_len][content_len][content: content_len]
*/

#include <types.h>

#define FNAME_MAX 32

typedef struct ram_file {
	struct ram_file* next;
	char name[FNAME_MAX];
	const char* content;
	int32_t size; 
} ram_file_t;

typedef struct {
	ram_file_t* head;
	const char* ram;
} ramfs_t;

void ramfs_open(const char*ram, ramfs_t* rd);
void ramfs_close(ramfs_t* rd);
/*
read file content of fname, return content address and size.
*/
const char* ramfs_read(ramfs_t* rd, const char* fname, int32_t* size);

#endif
