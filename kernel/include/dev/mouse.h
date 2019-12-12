#ifndef MOUSE_H
#define MOUSE_H

#include <dev/kdevice.h>

int32_t mouse_init(void);
int32_t mouse_handler(void);
int32_t mouse_dev_op(dev_t* dev, int32_t opcode, int32_t arg);

#endif
