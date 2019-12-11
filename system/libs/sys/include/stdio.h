#ifndef STDIO_H
#define STDIO_H

#include <fcntl.h>

void printf(const char *format, ...);
void uprintf(const char *format, ...);
void dprintf(int fd, const char *format, ...);

int getch(void);
void putch(int c);

#endif
