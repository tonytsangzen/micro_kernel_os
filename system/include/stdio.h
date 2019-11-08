#ifndef STDIO_H
#define STDIO_H

#include <fcntl.h>

extern int _stdin;
extern int _stdout;

void printf(const char *format, ...);
void dprintf(int fd, const char *format, ...);

int getch(void);
void putch(int c);

#endif
