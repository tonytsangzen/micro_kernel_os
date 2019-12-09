#ifndef UART_H
#define UART_H

#include <dev/kdevice.h>

int32_t uart_init(void);
int32_t uart_inputch(dev_t* dev, int32_t loop);
int32_t uart_write(dev_t* dev, const void* data, uint32_t size);

#endif
