#ifndef SD_H
#define SD_H

int sd_read(int block, void* buf); 
int sd_write(int block, const void* buf); 

#endif
