#ifndef UNISTD_H
#define UNISTD_H

#include <stdint.h>

int getpid(void);
int fork(void);
unsigned int sleep(unsigned int seconds);

#endif
