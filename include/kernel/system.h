#ifndef SYSTEM_H
#define SYSTEM_H

#include <types.h>

extern void __irq_enable(void);
extern void __irq_disable(void);
extern void __set_translation_table_base(uint32_t);

#endif
