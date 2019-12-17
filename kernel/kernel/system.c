#include <kernel/system.h>

inline void __attribute__((optimize("O0"))) _delay(uint32_t count) {
	while(count > 0) {
  	__asm__ volatile("nop");
		count--;
	}
}
