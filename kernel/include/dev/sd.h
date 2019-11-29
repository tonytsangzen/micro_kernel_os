#ifndef DEV_SD_H
#define DEV_SD_H

#include <dev/kdevice.h>

extern void sd_init(dev_t* dev);
extern int32_t sd_dev_read(dev_t* dev, int32_t bid);
extern int32_t sd_dev_read_done(dev_t* dev, void* buf);
extern int32_t sd_dev_write(dev_t* dev, int32_t bid, const void* buf);
extern int32_t sd_dev_write_done(dev_t* dev);
extern void sd_dev_handle(dev_t* dev);

#endif
