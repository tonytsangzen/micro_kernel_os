#ifndef BASIC_DEVICE_H
#define BASIC_DEVICE_H

#include <charbuf.h>

extern void dev_init(void);
extern charbuf_t* dev_getbuf(uint32_t type);

extern int32_t dev_read(uint32_t type, void* data, uint32_t size);
extern int32_t dev_write(uint32_t type, void* data, uint32_t size);

#endif
