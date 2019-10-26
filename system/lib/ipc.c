#include <ipc.h>
#include <svc_call.h>

int ipc_send_msg(int pid, void* data, uint32_t size) {
	return svc_call3(SYS_SEND_MSG, (int32_t)pid, (int32_t)data, (int32_t)size);
}

void* ipc_get_msg(int* from_pid,  uint32_t* size, uint8_t block) {
	return (void*)svc_call3(SYS_GET_MSG, (int32_t)from_pid, (int32_t)size, (int32_t)block);
}

