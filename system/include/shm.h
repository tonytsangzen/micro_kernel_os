#ifndef SHM_H
#define SHM_H

int   shm_alloc(unsigned int size, int flag);
void* shm_map(int shmid);
int   shm_unmap(int shmid);

#endif
