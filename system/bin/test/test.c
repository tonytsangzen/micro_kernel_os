#include <unistd.h>
#include <stdio.h>
#include <sys/critical.h>


int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	critical_enter();
	kprintf("enter\n");
	sleep(1);
	critical_quit();
	kprintf("quit\n");
  return 0;
}

