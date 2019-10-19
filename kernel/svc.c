#include <kernel/svc.h>

static int32_t svc_handler_raw(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx, int32_t processor_mode) {
	(void)code;
	(void)arg0;
	(void)arg1;
	(void)arg2;
	(void)ctx;
	(void)processor_mode;

	return 0;
}

void svc_handler(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx, int32_t processor_mode) {
	ctx->gpr[0] = svc_handler_raw(code, arg0, arg1, arg2, ctx, processor_mode);
}
