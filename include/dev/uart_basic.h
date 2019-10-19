#ifndef UART_BASIC_H
#define UART_BASIC_H

#include <stdint.h>

uint8_t uart_basic_init(void);

void uart_basic_putch(int c);

uint8_t uart_basic_ready_to_recv(void);

int32_t uart_basic_recv(void);

#endif
