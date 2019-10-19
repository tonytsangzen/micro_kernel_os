#include <svc_call.h>

int32_t svc_call(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2) {
	volatile int32_t r;
  __asm__ volatile("stmdb sp!, {lr}\n"
                "mrs   r0,  cpsr\n"
                "stmdb sp!, {r0}\n"
								"mov r0, %1 \n" // assign r0 =  code
								"mov r1, %2 \n" // assign r1 =  arg0
                "mov r2, %3 \n" // assign r2 =  arg1
                "mov r3, %4 \n" // assign r3 =  arg2
                "swi #0     \n" // make system call
                "mov %0, r0 \n" // assign r  = r0
                "ldmia sp!, {r1}\n"
                "ldmia sp!, {lr}\n"
              : "=r" (r)
              : "r"(code), "r" (arg0), "r" (arg1), "r" (arg2)
              : "r0", "r1", "r2", "r3" );
  return r;
}
