#include <sys/interrupt.h>
#include <stdlib.h>
#include <syscall.h>

static void proc_interrupt_entry(int int_id, interrupt_func_t func, void* p) {
	func(int_id, p);
	exit(0);
}

int proc_interrupt_register(interrupt_func_t func, void* p) {
	return syscall3(SYS_PROC_IRQ_REGISTER, (int32_t)proc_interrupt_entry, (int32_t)func, (int32_t)p);
}

void proc_interrupt(int pid, int int_id) {
	syscall2(SYS_PROC_INTERRUPT, (int32_t)pid, (int32_t)int_id);
}
