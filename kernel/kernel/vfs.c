#include <vfs.h>
#include <kstring.h>
#include <mm/kmalloc.h>
#include <kernel/proc.h>
#include <kstring.h>

static vfs_node_t* _vfs_root = NULL;
static mount_t _vfs_mounts[FS_MOUNT_MAX];

static void vfs_node_init(vfs_node_t* node) {
	memset(node, 0, sizeof(vfs_node_t));
	node->fsinfo.node = (uint32_t)node;
	node->fsinfo.mount_id = -1;
}

static void vfs_add(vfs_node_t* father, vfs_node_t* node) {
	if(father == NULL || node == NULL)
		return;

	node->father = father;
	node->fsinfo.mount_id = father->fsinfo.mount_id;
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

static int32_t vfs_get_mount_id(void) {
	int32_t i;
	for(i = 0; i<FS_MOUNT_MAX; i++) {
		if(_vfs_mounts[i].org_node == 0)
			return i;
	}
	return -1;
}

static void vfs_remove(vfs_node_t* node) {
	if(node == NULL)
		return;

	if(node->next != NULL)
		node->next->prev = node->prev;

	if(node->prev != NULL)
		node->prev->next = node->next;

	node->prev = NULL;
	node->next = NULL;
}

int32_t vfs_mount(vfs_node_t* org, vfs_node_t* node, uint32_t access) {
	if(org == NULL || node == NULL)
		return -1;
		
	if(node->fsinfo.mount_id > 0) //already been mounted 
		return -1;
	
	int32_t id = vfs_get_mount_id();
	if(id < 0)
		return -1;

	_vfs_mounts[id].access = access;
	_vfs_mounts[id].pid = _current_proc->pid;
	_vfs_mounts[id].org_node = (uint32_t)org;
	strcpy(_vfs_mounts[id].org_name, node->fsinfo.name);
	strcpy(node->fsinfo.name, org->fsinfo.name);
	node->fsinfo.mount_id =  id;

	vfs_node_t* father = org->father;
	if(father == NULL) {
		_vfs_root = node;	
	}
	else {
		vfs_remove(org);
		vfs_add(father, node);
	}
	return 0;
}

void vfs_umount(vfs_node_t* node) {
	if(node == NULL || node->fsinfo.mount_id < 0)
		return;

	strcpy(node->fsinfo.name, _vfs_mounts[node->fsinfo.mount_id].org_name);
	vfs_node_t* org = (vfs_node_t*)_vfs_mounts[node->fsinfo.mount_id].org_node;
	if(org == NULL)
		return;
	
	vfs_node_t* father = node->father;
	if(father == NULL) {
		_vfs_root = org;
	}
	else {
		vfs_remove(node);
		vfs_add(father, org);
	}
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
		if(strcmp(node->fsinfo.name, name) == 0) {
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
	char n[FS_FULL_NAME_MAX+1];
	int32_t j = 0;
	for(int32_t i=0; i<FS_FULL_NAME_MAX; i++) {
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
			node->fsinfo.type != FS_TYPE_DIR ||
			strchr(name, '/') != NULL)
		return NULL;

	strncpy(node->fsinfo.name, name, FS_NODE_NAME_MAX);
	vfs_add(father, node);
	return node;
}

vfs_node_t* vfs_root(void) {
	return _vfs_root;
}

void vfs_init(void) {
	int32_t i;
	for(i = 0; i<FS_MOUNT_MAX; i++) {
		memset(&_vfs_mounts[i], 0, sizeof(mount_t));
	}

	_vfs_root = vfs_new_node();
	strcpy(_vfs_root->fsinfo.name, "/");
}
