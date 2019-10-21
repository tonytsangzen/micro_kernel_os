#include <kernel/svc.h>
#include <kernel/schedule.h>
#include <kernel/proc.h>
#include <syscalls.h>
#include <kprintf.h>

static int32_t sys_exit(context_t* ctx, int32_t res) {
	if(_current_proc == NULL)
		return -1;
	proc_exit(ctx, _current_proc, res);
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

static int32_t svc_handler_raw(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx, int32_t processor_mode) {
	(void)arg1;
	(void)arg2;
	(void)ctx;
	(void)processor_mode;

	switch(code) {
	case SYS_UART_DEBUG: 
		return sys_uart_debug((const char*)arg0);		
	case SYS_EXIT:
		return sys_exit(ctx, arg0);
	case SYS_MALLOC:
		return sys_malloc(arg0);
	case SYS_FREE:
		return sys_free(arg0);
	case SYS_YIELD: 
		schedule(ctx);
		return 0;
	}
	return -1;
}

void svc_handler(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx, int32_t processor_mode) {
	ctx->gpr[0] = svc_handler_raw(code, arg0, arg1, arg2, ctx, processor_mode);
}
