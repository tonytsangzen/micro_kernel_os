#include <ipc.h>
#include <stddef.h>
#include <svc_call.h>

int ipc_send_msg(int pid, void* data, uint32_t size) {
	return svc_call3(SYS_SEND_MSG, (int32_t)pid, (int32_t)data, (int32_t)size);
}

void* ipc_get_msg(int* from_pid,  uint32_t* size, uint8_t block) {
	(void)block;
	return (void*)svc_call3(SYS_GET_MSG, (int32_t)from_pid, (int32_t)size, (int32_t)block);
}

int ipc_send_pkg(int to_pid, proto_t* pkg) {
	return ipc_send_msg(to_pid, pkg->data, pkg->size);
}

int ipc_get_pkg(int* from_pid,  proto_t* pkg, uint8_t block) {
	if(pkg == NULL)
		return -1;

	uint32_t size;
	void *data = ipc_get_msg(from_pid, &size, block);
	if(data == NULL)
		return -1;
	proto_copy(pkg, data, size);
	return 0;	
}

