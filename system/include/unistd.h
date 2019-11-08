#ifndef UNISTD_H
#define UNISTD_H

#include <stdint.h>

int getpid(void);
int fork(void);
unsigned int sleep(unsigned int seconds);

int read(int fd, void* buf, uint32_t size);
int write(int fd, const void* buf, uint32_t size);

int exec(const char* cmd_line);

#endif
