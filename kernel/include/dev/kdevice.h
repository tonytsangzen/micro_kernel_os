#ifndef BASIC_DEVICE_H
#define BASIC_DEVICE_H

#include <charbuf.h>

typedef struct st_char_dev {
	charbuf_t buffer;
	int32_t (*ready)(void);
	int32_t (*read)(void);
	int32_t (*write)(int32_t c);
} char_dev_t;

extern void char_dev_init(void);
extern char_dev_t* get_char_dev(uint32_t type);

extern int32_t char_dev_read(char_dev_t* dev, void* data, uint32_t size);
extern int32_t char_dev_write(char_dev_t* dev, void* data, uint32_t size);

#endif
