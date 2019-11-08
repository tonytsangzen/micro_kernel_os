#include <ramfs.h>
#include <mm/kmalloc.h>
#include <kstring.h>

void ramfs_close(ramfs_t* rd) {
	ram_file_t* rf = rd->head;
	while(rf != NULL) {
		rd->head = rf->next;
		kfree(rf);
		rf = rd->head;
	}
	rd->head = NULL;
}

void ramfs_open(const char*ram, ramfs_t* rd) {
	rd->ram = ram;
	rd->head = NULL;

	while(1) {
		//read name len
		int32_t name_len;
		memcpy(&name_len, ram, 4);
		if(name_len == 0) //end of disk
			break;
		ram += 4;
	
		//read name
		ram_file_t* rf = (ram_file_t*)kmalloc(sizeof(ram_file_t));
		memcpy(rf->name, ram, name_len);
		rf->name[name_len] = 0;
		ram += name_len;

		//read content len
		memcpy(&rf->size, ram, 4);
		ram += 4;
	
		//set content base
		rf->content = ram;
		ram += rf->size;

		rf->next = rd->head;
		rd->head = rf;
	}
}

/*
read file content of fname, return content address and size.
*/
const char* ramfs_read(ramfs_t* rd, const char* fname, int32_t* size) {
	if(rd == NULL)
		return NULL;

	ram_file_t* rf = rd->head;
	while(rf != NULL) {
		if(strcmp(fname, rf->name) == 0) {
			if(size != NULL)
				*size = rf->size;
			return rf->content;
		}
		rf = rf->next;
	}
	return NULL;
}

