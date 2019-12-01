#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vfs.h>
#include <sd.h>
#include <ext2fs.h>
#include <dev/device.h>

static void out(void* data, int32_t size) {
	char* buf = (char*)data;
	int32_t wr = 0;
	while(1) {
		if(size <= 0)
			break;

		int sz = write(1, buf, size);
		if(sz <= 0 && errno != EAGAIN)
			break;

		if(sz > 0) {
			size -= sz;
			wr += sz;
			buf += sz;
		}
	}
}

static int32_t add_nodes(ext2_t* ext2, INODE *ip, const char* dname) {
	int32_t i; 
	char c, *cp;
	DIR  *dp;
	char buf[BLOCK_SIZE];

	for (i=0; i<12; i++){
		if (ip->i_block[i] ){
			ext2->read_block(ip->i_block[i], buf);
			dp = (DIR *)buf;
			cp = buf;

			if(dp->inode == 0)
				continue;

			while (cp < &buf[BLOCK_SIZE]){
				c = dp->name[dp->name_len];  // save last byte
				dp->name[dp->name_len] = 0;   
				if(strcmp(dp->name, ".") != 0 && strcmp(dp->name, "..") != 0) {
					int32_t ino = dp->inode;
					INODE ip_node;
					if(ext2_node_by_ino(ext2, ino, &ip_node) == 0) {
						if(dp->file_type == 2) {//director
							uprintf("dir : %s/%s\n", dname, dp->name);
							str_t* dir_name = str_new(dname);
							if(strcmp(dname, "/") != 0)
								str_addc(dir_name, '/');
							str_add(dir_name, dp->name);
							add_nodes(ext2, &ip_node, CS(dir_name));
							str_free(dir_name);
						}
						else if(dp->file_type == 1) {//file
							uprintf("file: %s/%s, size=%d\n", dname, dp->name, ip_node.i_size);
						}
					}
				}
				//add node
				dp->name[dp->name_len] = c; // restore that last byte
				cp += dp->rec_len;
				dp = (DIR *)cp;
			}
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: test <ext2 fname>\n");
		return -1;
	}

	ext2_t ext2;
	ext2_init(&ext2, sd_read, sd_write);

	INODE root_node;
	ext2_node_by_fname(&ext2, "/", &root_node);
	add_nodes(&ext2, &root_node, "");

	int32_t sz;
	void* data = ext2_readfile(&ext2, argv[1], &sz);
	if(data != NULL) {
		out(data, sz);
		free(data);
	}

	ext2_quit(&ext2);
	return 0;
}
