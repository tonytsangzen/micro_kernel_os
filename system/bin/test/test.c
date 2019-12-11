#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <thread.h>
#include <proclock.h>

proc_lock_t _lock;

static void thread(void* data) {
	int *i = (int*)data;
	while((*i) < 10) {
		printf("thread: %d\n", *i);
		usleep(100000);

		proc_lock(_lock);
		(*i)++;
		proc_unlock(_lock);
	}
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	_lock = proc_lock_new();

	int i = 0;
	thread_create(thread, &i);
	while(i < 20) {
		printf("main: %d\n", i);
		usleep(100000);

		proc_lock(_lock);
		i++;
		proc_unlock(_lock);
	}

	proc_lock_free(_lock);
	return 0;
}
