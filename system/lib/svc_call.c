#include <svc_call.h>

int32_t svc_call3(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2) {
	volatile int32_t r;
	__asm__ volatile("stmdb sp!, {lr}");
	__asm__ volatile("mrs   r0,  cpsr");
	__asm__ volatile("stmdb sp!, {r0}");
	__asm__ volatile("stmia sp!, {r0-r3}");
	__asm__ volatile("ldr r0, %0" : : "m" (code));
	__asm__ volatile("ldr r1, %0" : : "m" (arg0));
	__asm__ volatile("ldr r2, %0" : : "m" (arg1));
	__asm__ volatile("ldr r3, %0" : : "m" (arg2));
	__asm__ volatile("ldmia sp!, {r0-r3}");
	__asm__ volatile("swi #0");
	__asm__ volatile("str r0, %0" : "=m" (r));
	__asm__ volatile("ldmia sp!, {r1}");
	__asm__ volatile("ldmia sp!, {lr}");
	return r;
}

int32_t svc_call2(int32_t code, int32_t arg0, int32_t arg1) {
	return svc_call3(code, arg0, arg1, 0);
}

int32_t svc_call1(int32_t code, int32_t arg0) {
	return svc_call3(code, arg0, 0, 0);
}

int32_t svc_call0(int32_t code) {
	return svc_call3(code, 0, 0, 0);
}
