#ifndef VFS_TREE_H
#define VFS_TREE_H

#include <types.h>

#define VFS_NODE_NAME_MAX 128
#define VFS_FULL_NAME_MAX 1024

#define VFS_TYPE_DIR   0
#define VFS_TYPE_FILE  1 

typedef struct vfs_node {
	uint32_t id;
	struct vfs_node* father; 
	struct vfs_node* first_kid; /*first child*/
	struct vfs_node* last_kid; /*last child*/
	struct vfs_node* next; /*next brother*/
	struct vfs_node* prev; /*prev brother*/
	
	uint32_t kids_num;

	char name[VFS_NODE_NAME_MAX];
	uint32_t type;
} vfs_node_t;

vfs_node_t* vfs_new_node(void);

vfs_node_t* vfs_simple_add(vfs_node_t* father, const char* name);

vfs_node_t* vfs_simple_get(vfs_node_t* father, const char* name);

vfs_node_t* vfs_get(vfs_node_t* father, const char* name);

void vfs_add(vfs_node_t* father, vfs_node_t* node);

void vfs_del(vfs_node_t* node);

void vfs_init(void);

vfs_node_t* vfs_root(void);

#endif
