#include <kernel/system.h>

inline void __attribute__((optimize("O0"))) _delay(uint32_t count) {
	while(count > 0) {
  	__asm__ volatile("nop");
		count--;
	}
}

inline void __attribute__((optimize("O0"))) _flush_tlb(void) {
  __asm__ volatile("MOV R6, #0");
  __asm__ volatile("MCR p15, 0, R6, c8, c7, 0");
}
