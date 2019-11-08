#ifndef UART_BASIC_H
#define UART_BASIC_H

#include <types.h>

int32_t uart_basic_init(void);

int32_t uart_basic_putch(int32_t c);

int32_t uart_basic_ready_to_recv(void);

int32_t uart_basic_recv(void);

#endif
