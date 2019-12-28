#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <dev/fbinfo.h>
#include <dev/kdevice.h>

extern char* _framebuffer_base;
extern char* _framebuffer_end;
extern char _framebuffer_base_raw[];
extern char _framebuffer_end_raw[];

extern int32_t fb_dev_init(uint32_t w, uint32_t h, uint32_t dep);
extern int32_t fb_dev_write(dev_t* dev, const void* buf, uint32_t size);
extern int32_t fb_dev_op(dev_t* dev, int32_t opcode, int32_t arg);

#endif
