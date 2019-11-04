#ifndef IPC_H
#define IPC_H

#include <proto.h>

int   ipc_send_pkg(int to_pid, const proto_t* pkg);
int   ipc_get_pkg(int* from_pid,  proto_t* pkg, uint8_t block);
int   ipc_recv(int* from_pid,  proto_t* pkg);
int   ipc_call(int to_pid,  const proto_t* ipkg, proto_t* opkg);

#endif
