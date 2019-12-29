#ifndef PRINTK_H
#define PRINTK_H

#include <console.h>
void uart_out(const char* s);
void printf(const char *format, ...);
console_t* get_console(void);

#endif
