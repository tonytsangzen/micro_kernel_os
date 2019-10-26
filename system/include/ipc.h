#ifndef IPC_H
#define IPC_H

#include <stdint.h>

int   ipc_send_msg(int to_pid, void* data, uint32_t size);
void* ipc_get_msg(int* from_pid,  uint32_t* size, uint8_t block);

#endif
