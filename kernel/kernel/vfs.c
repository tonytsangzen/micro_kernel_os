#include <vfs.h>
#include <kstring.h>
#include <mm/kmalloc.h>

static uint32_t _node_id_counter = 0;
static vfs_node_t* _vfs_root = NULL;

static void vfs_node_init(vfs_node_t* node) {
	memset(node, 0, sizeof(vfs_node_t));
	node->id = _node_id_counter++;
}

void vfs_add(vfs_node_t* father, vfs_node_t* node) {
	if(father == NULL || node == NULL)
		return;

	node->father = father;
	if(father->last_kid == NULL) {
		father->first_kid = node;
	}
	else {
		father->last_kid->next = node;
		node->prev = father->last_kid;
	}
	father->kids_num++;
	father->last_kid = node;
}

void vfs_del(vfs_node_t* node) {
	if(node == NULL)
		return;
	/*free children*/
	vfs_node_t* c = node->first_kid;
	while(c != NULL) {
		vfs_node_t* next = c->next;
		vfs_del(c);
		c = next;
	}

	vfs_node_t* father = node->father;
	if(father != NULL) {
		if(father->first_kid == node)
			father->first_kid = node->next;
		if(father->last_kid == node)
			father->last_kid = node->prev;
		father->kids_num--;
	}

	if(node->next != NULL)
		node->next->prev = node->prev;
	if(node->prev != NULL)
		node->prev->next = node->next;
	kfree(node);
}

vfs_node_t* vfs_new_node(void) {
	vfs_node_t* ret = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
	vfs_node_init(ret);
	return ret;
}

vfs_node_t* vfs_simple_get(vfs_node_t* father, const char* name) {
	if(father == NULL || strchr(name, '/') != NULL)
		return NULL;

	vfs_node_t* node = father->first_kid;
	while(node != NULL) {
		if(strcmp(node->name, name) == 0) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

vfs_node_t* vfs_get(vfs_node_t* father, const char* name) {
	if(father == NULL)
		return NULL;
	
	if(name[0] == '/') {
		/*go to root*/
		while(father->father != NULL)
			father = father->father;

		name = name+1;
		if(name[0] == 0)
			return father;
	}

	vfs_node_t* node = father;	
	char n[VFS_FULL_NAME_MAX+1];
	int32_t j = 0;
	for(int32_t i=0; i<VFS_FULL_NAME_MAX; i++) {
		n[i] = name[i];
		if(n[i] == 0) {
			return vfs_simple_get(node, n+j);
		}
		if(n[i] == '/') {
			n[i] = 0; 
			node = vfs_simple_get(node, n+j);
			if(node == NULL)
				return NULL;
			j= i+1;
		}
	}
	return NULL;
}
	
vfs_node_t* vfs_simple_add(vfs_node_t* father, const char* name) {
	vfs_node_t* node = vfs_new_node();
	if(node == NULL ||
			node->type != VFS_TYPE_DIR ||
			strchr(name, '/') != NULL)
		return NULL;

	strncpy(node->name, name, VFS_NODE_NAME_MAX);
	vfs_add(father, node);
	return node;
}

vfs_node_t* vfs_root(void) {
	return _vfs_root;
}

void vfs_init(void) {
	_vfs_root = vfs_new_node();
}
