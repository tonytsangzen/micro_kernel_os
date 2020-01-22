#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>

typedef void(*interrupt_func_t)(int int_id, void* p);

int proc_interrupt_register(interrupt_func_t func, void* p);

void proc_interrupt(int pid, int int_id);

#endif
