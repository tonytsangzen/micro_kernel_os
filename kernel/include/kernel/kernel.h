#ifndef KERNEL_H
#define KERNEL_H

#include <mm/mmu.h>
  
extern char _kernel_start[];
extern char _kernel_end[];

#ifdef _INITRD
extern char _initrd[];
#endif

extern page_dir_entry_t* _kernel_vm;
extern void set_kernel_vm(page_dir_entry_t* vm);

extern uint32_t _kernel_tic;

#endif
