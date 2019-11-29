#ifndef DEV_SDC_H
#define DEV_SDC_H

#include <dev/kdevice.h>

extern void sdc_init(dev_t* dev);
extern int32_t sdc_dev_read(dev_t* dev, int32_t bid);
extern int32_t sdc_dev_read_done(dev_t* dev, void* buf);
extern int32_t sdc_dev_write(dev_t* dev, int32_t bid, const void* buf);
extern int32_t sdc_dev_write_done(dev_t* dev);
extern void sdc_dev_handle(dev_t* dev);

#endif
