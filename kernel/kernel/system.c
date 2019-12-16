#include <kernel/system.h>

inline void __attribute__((optimize("O0"))) _delay(uint32_t count) {
  __asm__ volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
      : : [count]"r"(count) : "cc");
}

inline void  __attribute__((optimize("O0"))) _delay_msec(uint32_t n) {
	_delay(n * 500000);
}

