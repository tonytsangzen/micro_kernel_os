#ifndef SVC_CALL_H
#define SVC_CALL_H

#include <stdint.h>

extern int32_t svc_call3(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2);
extern int32_t svc_call2(int32_t code, int32_t arg0, int32_t arg1);
extern int32_t svc_call1(int32_t code, int32_t arg0);
extern int32_t svc_call0(int32_t code);

#endif
