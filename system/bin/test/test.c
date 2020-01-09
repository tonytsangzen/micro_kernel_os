#include <unistd.h>
#include <stdio.h>
#include <sys/critical.h>


int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	critical_enter();
	kprintf("enter\n");
	sleep(1);
	kprintf("quit\n");
	critical_quit();
	//sleep(1);
  return 0;
}

