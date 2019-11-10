#ifndef KEYB_H
#define KEYB_H

#include <dev/kdevice.h>

int32_t keyb_init(void);
int32_t keyb_inputch(dev_t* dev, int32_t loop);
int32_t keyb_outputch(dev_t* dev, int32_t c);

#endif
