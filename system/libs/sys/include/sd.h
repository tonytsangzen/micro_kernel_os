#ifndef SD_H
#define SD_H

#include <stdint.h>

int32_t sd_read(int32_t block, void* buf); 
int32_t sd_write(int32_t block, const void* buf); 
int32_t sd_init(void);
int32_t sd_quit(void);
int32_t sd_set_buffer(uint32_t sector_num);

#endif
