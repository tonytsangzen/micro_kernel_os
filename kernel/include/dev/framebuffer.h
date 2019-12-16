#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <dev/fbinfo.h>
#include <dev/kdevice.h>

extern char _framebuffer_base[];
extern char _framebuffer_end[];

extern int32_t fb_dev_init(int32_t res);
extern uint32_t fb_dev_get_size(void);
extern int32_t fb_dev_write(dev_t* dev, const void* buf, uint32_t size);
extern int32_t fb_dev_op(dev_t* dev, int32_t opcode, int32_t arg);

#endif
