#ifndef ARCH_INFO_H
#define ARCH_INFO_H

#include <types.h>

typedef struct {
	uint32_t phy_mem_size;
	uint32_t phy_mmio_base;
	uint32_t mmio_size;
} arch_info_t;

extern arch_info_t _arch_info;

extern void arch_info_init(void);

#endif
