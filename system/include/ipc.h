#ifndef IPC_H
#define IPC_H

#include <proto.h>

int   ipc_send_raw(int pid, void* data, uint32_t size);
int   ipc_send(int to_pid, const proto_t* pkg);

void* ipc_get_raw(int* from_pid, uint32_t* size, uint8_t block);
int   ipc_get(int* from_pid,  proto_t* pkg, uint8_t block);
int   ipc_recv(int* from_pid,  proto_t* pkg);

void* ipc_get_from_raw(int from_pid, uint32_t* size, uint8_t block);
int   ipc_get_from(int from_pid,  proto_t* pkg, uint8_t block);
int   ipc_recv_from(int from_pid,  proto_t* pkg);

int   ipc_call(int to_pid,  const proto_t* ipkg, proto_t* opkg);

#endif
