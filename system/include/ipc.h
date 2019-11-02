#ifndef IPC_H
#define IPC_H

#include <proto.h>

int   ipc_send_pkg(int to_pid, proto_t* pkg);
int   ipc_get_pkg(int* from_pid,  proto_t* pkg, uint8_t block);

#endif
