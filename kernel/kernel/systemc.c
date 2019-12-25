#include <kernel/system.h>
#include <dev/actled.h>

extern void __dummy(void);
inline void _delay(uint32_t count) {
	while(count > 0) {
		__dummy();
		count--;
	}
}

void flush_led(void) {
	act_led(true);
	_delay(0x100000);
	act_led(false);
	_delay(0x100000);
}

inline void __attribute__((optimize("O0"))) _flush_tlb(void) {
  __asm__ volatile("MOV R6, #0");
  __asm__ volatile("MCR p15, 0, R6, c8, c7, 0");
}
