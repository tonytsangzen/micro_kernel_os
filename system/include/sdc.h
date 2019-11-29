#ifndef SDC_H
#define SDC_H

int sdc_read(int block, void* buf); 
int sdc_write(int block, const void* buf); 

#endif
