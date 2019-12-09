#include <global.h>
#include <stdio.h>
#include <unistd.h>
#include <cmain.h>

int main(int argc, char* argv[]) {
	printf("switch to X.\n");
	set_global("current_console", "x");
	int i;
	for(i=1; i<argc; i++) {
		printf("run '%s'.\nn", argv[i]);
		int pid = fork();
		if(pid == 0) {
			detach();
			exec(argv[i]);
		}
	}

	return 0;
}
