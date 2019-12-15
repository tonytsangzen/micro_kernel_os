#include <kernel/system.h>

inline void _delay(int32_t count) {
  __asm__ volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
      : : [count]"r"(count) : "cc");
}
