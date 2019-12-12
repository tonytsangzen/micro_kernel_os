#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	(void)argc;
	FILE* fp = fopen(argv[1], "r");
	char buf[16+1];
	int i = fread(buf, 8, 2, fp);
	buf[i*8] = 0;
	printf("%d: [%s]\n", i, buf);
	fclose(fp);
  return 0;
}

