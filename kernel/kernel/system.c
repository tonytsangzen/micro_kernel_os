#include <kernel/system.h>

inline void __attribute__((optimize("O0"))) _delay(uint32_t count) {
	while(count > 0) {
  	__asm__ volatile("nop");
		count--;
	}
}

inline void  __attribute__((optimize("O0"))) _delay_msec(uint32_t n) {
	_delay(n * 100000);
}

