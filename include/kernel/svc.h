#ifndef SVC_H
#define SVC_H

#include <kernel/context.h>

extern int32_t svc_handler(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx, int32_t processor_mode);

#endif
