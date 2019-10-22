#include <kernel/svc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <kernel/proc.h>
#include <syscalls.h>
#include <kstring.h>
#include <kprintf.h>

static int32_t sys_exit(context_t* ctx, int32_t res) {
	if(_current_proc == NULL)
		return -1;
	proc_exit(ctx, _current_proc, res);
	return 0;
}

static int32_t sys_getpid(void) {
	if(_current_proc == NULL)
		return -1;
	return _current_proc->pid;
}

static int32_t sys_sleep_on(context_t* ctx, uint32_t event) {
	proc_sleep(ctx, event);
	return 0;
}

static int32_t sys_wakeup(uint32_t event) {
	proc_wakeup(event);
	return 0;
}

static int32_t sys_uart_debug(const char* s) {
	printf("%s", s);
	return 0;
}

static int32_t sys_malloc(int32_t size) {
	return (int32_t)proc_malloc(_current_proc, size);
}

static int32_t sys_free(int32_t p) {
	if(p == 0)
		return -1;
	proc_free(_current_proc, (void*)p);
	return 0;
}

static int32_t sys_fork(void) {
	proc_t *proc = kfork();
	return proc->pid;
}

static int32_t svc_handler_raw(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx, int32_t processor_mode) {
	(void)arg1;
	(void)arg2;
	(void)ctx;
	(void)processor_mode;

	if(_current_proc != NULL)
		memcpy(&_current_proc->ctx, ctx, sizeof(context_t));

	switch(code) {
	case SYS_UART_DEBUG: 
		return sys_uart_debug((const char*)arg0);		
	case SYS_EXIT:
		return sys_exit(ctx, arg0);
	case SYS_MALLOC:
		return sys_malloc(arg0);
	case SYS_FREE:
		return sys_free(arg0);
	case SYS_GET_PID:
		return sys_getpid();
	case SYS_SLEEP_ON:
		return sys_sleep_on(ctx, (uint32_t)arg0);
	case SYS_WAKEUP:
		return sys_wakeup((uint32_t)arg0);
	case SYS_FORK:
		return sys_fork();
	case SYS_YIELD: 
		schedule(ctx);
		return 0;
	}
	return -1;
}

void svc_handler(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx, int32_t processor_mode) {
	__irq_disable();
	ctx->gpr[0] = svc_handler_raw(code, arg0, arg1, arg2, ctx, processor_mode);
}
