#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#define FULL_MAX 512

void dump(FILE* out, const char*dname, const char* fname) {
	char full[FULL_MAX];
	snprintf(full, FULL_MAX-1, "%s/%s", dname, fname);
	FILE* f = fopen(full, "r");
	if(f == NULL)
		return;
	
	//write name info
	int32_t nameLen = strlen(fname)+1;
	fwrite(&nameLen, 1, 4, out);
	fwrite("/", 1, 1, out);
	fwrite(fname, 1, nameLen-1, out);

	//write content info
	fseek(f, 0, SEEK_END); // seek to end of file
	int32_t size = ftell(f); // get current file pointer
	fseek(f, 0, SEEK_SET); // seek back to beginning of file
	fwrite(&size, 1, 4, out);

	char* p = malloc(size);
	int res = fread(p, 1, size, f);
	res = fwrite(p, 1, size, out);

	free(p);
	fclose(f);
}


int dump_dir(FILE* fp, const char* root, const char* dn) {
	char fname[FULL_MAX];
	if(dn[0] == 0)
		strncpy(fname, root, FULL_MAX-1);
	else
		snprintf(fname, FULL_MAX-1, "%s/%s", root, dn);

	DIR* dir = opendir(fname);
	if(dir == NULL)
		return -2;

	while(1) {
		struct dirent* r = readdir(dir);
		if(r == NULL)
			break;

		if(r->d_name[0] == '.')
			continue;
		if(dn[0] == 0)
			strncpy(fname, r->d_name, FULL_MAX-1);
		else
			snprintf(fname, FULL_MAX-1, "%s/%s", dn, r->d_name);

		if(r->d_type == DT_DIR)
			dump_dir(fp, root, fname);
		else 
			dump(fp, root, fname);
	}

	closedir(dir);
	return 0;
}
/*
mkramfs [output_file] [source_dir]
*/
int main(int argc, char** argv) {
	if(argc < 3) {
		printf("Usage: mkramfs <ramfs_file> <path>\n");
		return -1;
	}

	FILE* fp = fopen(argv[1], "w");
	if(fp == NULL) {
		return -1;
	}

	dump_dir(fp, argv[2], "");

	int32_t end = 0;
	fwrite(&end, 1, 4, fp);
	fclose(fp);
	return 0;
}
