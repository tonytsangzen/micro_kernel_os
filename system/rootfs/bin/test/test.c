#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <thread.h>

static void thread(void* data) {
	int *i = (int*)data;
	while(1) {
		printf("thread: %d\n", *i);
		sleep(1);
	}
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int i = 0;
	thread_create(thread, &i);
	while(1) {
		printf("main: %d\n", i++);
		sleep(1);
	}
	return 0;
}
