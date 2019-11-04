#include <ipc.h>
#include <stddef.h>
#include <svc_call.h>

int ipc_send_msg(int pid, void* data, uint32_t size) {
	return svc_call3(SYS_SEND_MSG, (int32_t)pid, (int32_t)data, (int32_t)size);
}

void* ipc_get_msg(int* from_pid,  uint32_t* size, uint8_t block) {
	return (void*)svc_call3(SYS_GET_MSG, (int32_t)from_pid, (int32_t)size, (int32_t)block);
}

int ipc_send_pkg(int to_pid, const proto_t* pkg) {
	if(to_pid < 0 || pkg == NULL)
		return -1;
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

int ipc_recv(int* from_pid,  proto_t* pkg) {
	if(pkg == NULL)
		return -1;
	proto_clear(pkg);

	while(1) {
		int res = ipc_get_pkg(from_pid, pkg, 1);
		if(res == 0)
			return 0;
	}
	return -1;
}

int ipc_call(int to_pid,  const proto_t* ipkg, proto_t* opkg) {
	if(to_pid < 0 || ipkg == NULL)
		return -1;

	if(ipc_send_pkg(to_pid, ipkg) != 0)
		return -1;

	if(opkg == NULL)
		return 0;

	if(ipc_recv(NULL, opkg) == 0)
		return 0;
	return -1;
}
