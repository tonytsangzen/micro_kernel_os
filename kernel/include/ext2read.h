#ifndef EXT2_FS_READ_H
#define EXT2_FS_READ_H

#include <types.h>

void* sd_read_ext2(const char* fname, int32_t* sz); 

#endif
