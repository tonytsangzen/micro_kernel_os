#ifndef UART_H
#define UART_H

#include <dev/kdevice.h>

int32_t uart_dev_init(void);
int32_t uart_inputch(dev_t* dev, int32_t loop);
int32_t uart_write(dev_t* dev, const void* data, uint32_t size);
int32_t uart_ready_to_recv(void);
int32_t uart_recv(void);

#endif
