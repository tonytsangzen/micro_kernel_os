#ifndef BASIC_DEVICE_H
#define BASIC_DEVICE_H

#include <charbuf.h>
#include <dev/device.h>

typedef struct st_dev {
	int32_t type;
	charbuf_t buffer;

	int32_t (*ready)(struct st_dev* p);
	int32_t (*inputch)(struct st_dev* dev, int32_t loop);
	int32_t (*outputch)(struct st_dev* dev, int32_t c);
	int32_t (*read)(struct st_dev* dev, void* buf, uint32_t size);
	int32_t (*write)(struct st_dev* dev, const void* buf, uint32_t size);
	int32_t (*op)(struct st_dev* dev, int32_t opcode, int32_t arg);
} dev_t;

extern void    dev_init(void);
extern dev_t*  get_dev(uint32_t type);
extern int32_t dev_ready(dev_t* dev);
extern int32_t dev_op(dev_t* dev, int32_t opcode, int32_t arg);

/*return : -1 for error/closed, 0 for retry*/
extern int32_t dev_read(dev_t* dev, void* data, uint32_t size);
/*return : -1 for error/closed, 0 for retry*/
extern int32_t dev_write(dev_t* dev, void* data, uint32_t size);

#endif
