#ifndef IPC_H
#define IPC_H

#include <proto.h>

int   ipc_send(int to_pid, const proto_t* pkg, int32_t id);
int   ipc_get(int* from_pid,  proto_t* pkg, int32_t id, uint8_t block);

int   ipc_call(int to_pid,  const proto_t* ipkg, proto_t* opkg);

#endif
