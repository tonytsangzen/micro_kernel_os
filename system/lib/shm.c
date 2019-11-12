#include <shm.h>
#include <svc_call.h>

int shm_alloc(unsigned int size, int flag) {
	return svc_call2(SYS_PROC_SHM_ALLOC, size, flag);
}

void* shm_map(int shmid) {
	return (void*)svc_call1(SYS_PROC_SHM_MAP, shmid);
}

int shm_unmap(int shmid) {
	return svc_call1(SYS_PROC_SHM_UNMAP, shmid);
}
