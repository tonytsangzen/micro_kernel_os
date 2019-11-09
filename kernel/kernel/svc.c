#include <kernel/kernel.h>
#include <kernel/svc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <kernel/proc.h>
#include <vfs.h>
#include <syscalls.h>
#include <kstring.h>
#include <kprintf.h>
#include <dev/kdevice.h>

static void sys_exit(context_t* ctx, int32_t res) {
	if(_current_proc == NULL)
		return;
	proc_exit(ctx, _current_proc, res);
}

static int32_t sys_char_dev_write(uint32_t type, void* data, uint32_t sz) {
	char_dev_t* dev = get_char_dev(type);
	if(dev == NULL)
		return -1;
	return char_dev_write(dev, data, sz);
}

static void sys_char_dev_read(context_t* ctx, uint32_t type, void* data, uint32_t sz) {
	char_dev_t* dev = get_char_dev(type);
	if(dev == NULL) {
		ctx->gpr[0] = -1;
		return;
	}

	int32_t rd = char_dev_read(dev, data, sz);
	if(rd > 0) {
		ctx->gpr[0] = rd;
		return;
	}

	proc_t* proc = _current_proc;
	proc_sleep_on(ctx, (uint32_t)dev);
	proc->ctx.gpr[0] = 0;
}

static int32_t sys_getpid(void) {
	if(_current_proc == NULL)
		return -1;
	return _current_proc->pid;
}

static void sys_sleep_on(context_t* ctx, uint32_t event) {
	proc_sleep_on(ctx, event);
}

static void sys_wakeup(uint32_t event) {
	proc_wakeup(event);
}

static int32_t sys_malloc(int32_t size) {
	return (int32_t)proc_malloc(size);
}

static void sys_free(int32_t p) {
	if(p == 0)
		return;
	proc_free((void*)p);
}

static int32_t sys_fork(context_t* ctx) {
	proc_t *proc = kfork();
	if(proc == NULL)
		return -1;

	memcpy(&proc->ctx, ctx, sizeof(context_t));
	proc->ctx.gpr[0] = 0;
	return proc->pid;
}

static void sys_waitpid(context_t* ctx, int32_t pid) {
	proc_waitpid(ctx, pid);
}

static void get_fsinfo(vfs_node_t* node, fsinfo_t* info) {
	memcpy(info, &node->fsinfo, sizeof(fsinfo_t));
}

static int32_t sys_vfs_get_info(const char* name, fsinfo_t* info) {
	vfs_node_t* node = vfs_get(vfs_root(), name);
	if(node == NULL)
		return -1;
	get_fsinfo(node, info);
	return 0;
}

static int32_t sys_vfs_get_fkid(fsinfo_t* info, fsinfo_t* ret) {
	vfs_node_t* node = (vfs_node_t*)info->node;
	if(node == NULL || node->first_kid == NULL || ret == NULL)
		return -1;
	get_fsinfo(node->first_kid, ret);
	return 0;
}

static int32_t sys_vfs_get_next(fsinfo_t* info, fsinfo_t* ret) {
	vfs_node_t* node = (vfs_node_t*)info->node;
	if(node == NULL || node->next == NULL || ret == NULL)
		return -1;
	get_fsinfo(node->next, ret);
	return 0;
}

static int32_t sys_vfs_get_father(fsinfo_t* info, fsinfo_t* ret) {
	vfs_node_t* node = (vfs_node_t*)info->node;
	if(node == NULL || node->father == NULL || ret == NULL)
		return -1;
	get_fsinfo(node->father, ret);
	return 0;
}

static int32_t sys_vfs_set_info(fsinfo_t* info) {
	if(info == NULL)
		return -1;
	vfs_node_t* node  = (vfs_node_t*)info->node;
	if(node == NULL)
		return -1;
	return vfs_set(node, info);
}

static int32_t sys_vfs_add(fsinfo_t* info_to, fsinfo_t* info) {
	if(info_to == NULL || info == NULL)
		return -1;
	
	vfs_node_t* node_to = (vfs_node_t*)info_to->node;
	vfs_node_t* node = (vfs_node_t*)info->node;
	if(node_to == NULL || node == NULL)
		return -1;

	return vfs_add(node_to, node);
}

static int32_t sys_vfs_get_mount(fsinfo_t* info, mount_t* mount) {
	if(info == NULL || mount == NULL)
		return -1;
	
	vfs_node_t* node = (vfs_node_t*)info->node;
	if(node == NULL)
		return -1;
	
	return vfs_get_mount(node, mount);
}

static int32_t sys_vfs_mount(fsinfo_t* info_to, fsinfo_t* info, mount_info_t* mnt_info) {
	if(info_to == NULL || info == NULL || mnt_info == NULL)
		return -1;
	
	vfs_node_t* node_to = (vfs_node_t*)info_to->node;
	vfs_node_t* node = (vfs_node_t*)info->node;
	if(node == NULL || node_to == NULL)
		return -1;
	
	vfs_mount(node_to, node, mnt_info);
	return 0;
}

static void sys_vfs_umount(fsinfo_t* info) {
	if(info == NULL)
		return;
	
	vfs_node_t* node = (vfs_node_t*)info->node;
	if(node == NULL)
		return;
	
	vfs_umount(node);
}

static int32_t sys_vfs_open(int32_t pid, fsinfo_t* info, int32_t wr) {
	if(info == NULL || pid < 0)
		return -1;
	
	vfs_node_t* node = (vfs_node_t*)info->node;
	if(node == NULL)
		return -1;
	
	return vfs_open(pid, node, wr);
}

static void sys_vfs_close(int32_t fd) {
	if(fd < 0)
		return;
	vfs_close(_current_proc, fd);
}

static int32_t sys_vfs_tell(int32_t fd) {
	if(fd < 0)
		return -1;
	return vfs_tell(fd);
}

static int32_t sys_vfs_new_node(fsinfo_t* info) {
	if(info  == NULL)
		return -1;

	vfs_node_t* node = vfs_new_node();
	if(node == NULL)
		return -1;
	info->node = (uint32_t)node;
	info->mount_id = -1;

	memcpy(&node->fsinfo, info, sizeof(fsinfo_t));
	return 0;
}

static int32_t sys_vfs_del(fsinfo_t* info) {
	if(info == NULL)
		return -1;
		
	vfs_node_t* node = (vfs_node_t*)info->node;
	if(node == NULL)
		return -1;

	vfs_del(node);
	return 0;
}

static int32_t sys_vfs_get_by_fd(int32_t fd, fsinfo_t* info) {
	if(fd < 0 || info == NULL)
		return -1;
	
	vfs_node_t* node = vfs_node_by_fd(fd);
	if(node == NULL)
		return -1;

	memcpy(info, &node->fsinfo, sizeof(fsinfo_t));
	return 0;
}

static int32_t sys_load_elf(context_t* ctx, const char* cmd, void* elf, uint32_t elf_size) {
	if(elf == NULL)
		return -1;
	
	tstr_cpy(_current_proc->cmd, cmd);
	if(proc_load_elf(_current_proc, elf, elf_size) != 0)
		return -1;

	memcpy(ctx, &_current_proc->ctx, sizeof(context_t));
	return 0;
}

static void sys_get_msg(context_t* ctx, int32_t *pid, uint32_t* size, int32_t block) {
	void* p = proc_get_msg(pid, size);
	if(p != NULL) {
		ctx->gpr[0] = (uint32_t)p;
		return;
	}

	if(block == 0) { //non-block mode
		ctx->gpr[0] = 0;
		return;
	}

	proc_t* proc = _current_proc;
	proc_sleep_on(ctx, (uint32_t)&proc->pid);
	proc->ctx.gpr[0] = 0;
}

static int32_t sys_proc_set_cwd(const char* cwd) {
	tstr_cpy(_current_proc->cwd, cwd);
	return 0;
}

static void sys_proc_get_cwd(char* cwd, int32_t sz) {
	strncpy(cwd, CS(_current_proc->cwd), sz);
}

static inline const char* syscall_code(int32_t code) {
	switch(code) {
	case SYS_CHAR_DEV_READ:
		return "SYS_CHAR_DEV_READ";
	case SYS_CHAR_DEV_WRITE:
		return "SYS_CHAR_DEV_WRITE";
	case SYS_INITRD:
		return "SYS_INITRD";
	case SYS_EXIT:
		return "SYS_EXIT";
	case SYS_MALLOC:
		return "SYS_MALLOC";
	case SYS_FREE:
		return "SYS_FREE";
	case SYS_GET_PID:
		return "SYS_GET_PID";
	case SYS_SLEEP_ON:
		return "SYS_SLEEP_ON";
	case SYS_WAKEUP:
		return "SYS_WAKEUP";
	case SYS_EXEC_ELF:
		return "SYS_EXEC_ELF";
	case SYS_FORK:
		return "SYS_FORK";
	case SYS_WAIT_PID:
		return "SYS_WAIT_PID";
	case SYS_SEND_MSG:
		return "SYS_SEND_MSG";
	case SYS_GET_MSG:
		return "SYS_GET_MSG";
	case SYS_VFS_GET:
		return "SYS_VFS_GET";
	case SYS_VFS_FKID:
		return "SYS_VFS_FKID";
	case SYS_VFS_NEXT:
		return "SYS_VFS_NEXT";
	case SYS_VFS_FATHER:
		return "SYS_VFS_FATHER";
	case SYS_VFS_SET:
		return "SYS_VFS_SET";
	case SYS_VFS_ADD:
		return "SYS_VFS_ADD";
	case SYS_VFS_DEL:
		return "SYS_VFS_DEL";
	case SYS_VFS_NEW_NODE:
		return "SYS_VFS_NEW_NODE";
	case SYS_VFS_GET_MOUNT:
		return "SYS_VFS_GET_MOUNT";
	case SYS_VFS_MOUNT:
		return "SYS_VFS_MOUNT";
	case SYS_VFS_UMOUNT:
		return "SYS_VFS_UMOUNT";
	case SYS_VFS_OPEN:
		return "SYS_VFS_OPEN";
	case SYS_VFS_PROC_CLOSE:
		return "SYS_VFS_PROC_CLOSE";
	case SYS_VFS_PROC_SEEK:
		return "SYS_VFS_PROC_SEEK";
	case SYS_VFS_PROC_TELL:
		return "SYS_VFS_PROC_TELL";
	case SYS_VFS_PROC_GET_BY_FD:
		return "SYS_VFS_PROC_GET_BY_FD";
	case SYS_YIELD: 
		return "SYS_YIELD";
	}
	return "NONE";
}

void svc_handler(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx, int32_t processor_mode) {
	(void)arg1;
	(void)arg2;
	(void)ctx;
	(void)processor_mode;
	
	__irq_disable();
	//printf("pid:%d, code: %d (%s)\n", _current_proc->pid, code, syscall_code(code));

	switch(code) {
	case SYS_CHAR_DEV_READ:
		sys_char_dev_read(ctx, arg0, (void*)arg1, arg2);		
		return;
	case SYS_CHAR_DEV_WRITE:
		ctx->gpr[0] = sys_char_dev_write(arg0, (void*)arg1, arg2);		
		return;
	case SYS_INITRD:
		ctx->gpr[0] = (int32_t)_initrd;
		return;
	case SYS_EXIT:
		sys_exit(ctx, arg0);
		return;
	case SYS_MALLOC:
		ctx->gpr[0] = sys_malloc(arg0);
		return;
	case SYS_FREE:
		sys_free(arg0);
		return;
	case SYS_GET_PID:
		ctx->gpr[0] = sys_getpid();
		return;
	case SYS_SLEEP_ON:
		sys_sleep_on(ctx, (uint32_t)arg0);
		return;
	case SYS_WAKEUP:
		sys_wakeup((uint32_t)arg0);
		return;
	case SYS_EXEC_ELF:
		ctx->gpr[0] = sys_load_elf(ctx, (const char*)arg0, (void*)arg1, (uint32_t)arg2);
		return;
	case SYS_FORK:
		ctx->gpr[0] = sys_fork(ctx);
		return;
	case SYS_WAIT_PID:
		sys_waitpid(ctx, arg0);
		return;
	case SYS_SEND_MSG:
		ctx->gpr[0] = proc_send_msg(arg0, (void*)arg1, (uint32_t)arg2);
		return;
	case SYS_GET_MSG:
		sys_get_msg(ctx, (int32_t*)arg0, (uint32_t*)arg1, arg2);
		return;
	case SYS_VFS_GET:
		ctx->gpr[0] = sys_vfs_get_info((const char*)arg0, (fsinfo_t*)arg1);
		return;
	case SYS_VFS_FKID:
		ctx->gpr[0] = sys_vfs_get_fkid((fsinfo_t*)arg0, (fsinfo_t*)arg1);
		return;
	case SYS_VFS_NEXT:
		ctx->gpr[0] = sys_vfs_get_next((fsinfo_t*)arg0, (fsinfo_t*)arg1);
		return;
	case SYS_VFS_FATHER:
		ctx->gpr[0] = sys_vfs_get_father((fsinfo_t*)arg0, (fsinfo_t*)arg1);
		return;
	case SYS_VFS_SET:
		ctx->gpr[0] = sys_vfs_set_info((fsinfo_t*)arg0);
		return;
	case SYS_VFS_ADD:
		ctx->gpr[0] = sys_vfs_add((fsinfo_t*)arg0, (fsinfo_t*)arg1);
		return;
	case SYS_VFS_DEL:
		ctx->gpr[0] = sys_vfs_del((fsinfo_t*)arg0);
		return;
	case SYS_VFS_NEW_NODE:
		ctx->gpr[0] = (int32_t)sys_vfs_new_node((fsinfo_t*)arg0);
		return;
	case SYS_VFS_GET_MOUNT:
		ctx->gpr[0] = sys_vfs_get_mount((fsinfo_t*)arg0, (mount_t*)arg1);
		return;
	case SYS_VFS_MOUNT:
		ctx->gpr[0] = sys_vfs_mount((fsinfo_t*)arg0, (fsinfo_t*)arg1, (mount_info_t*)arg2);
		return;
	case SYS_VFS_UMOUNT:
		sys_vfs_umount((fsinfo_t*)arg0);
		return;
	case SYS_VFS_OPEN:
		ctx->gpr[0] = sys_vfs_open(arg0, (fsinfo_t*)arg1, arg2);
		return;
	case SYS_VFS_PROC_CLOSE:
		sys_vfs_close(arg0);
		return;
	case SYS_VFS_PROC_SEEK:
		ctx->gpr[0] = vfs_seek(arg0, arg1);
		return;
	case SYS_VFS_PROC_TELL:
		ctx->gpr[0] = sys_vfs_tell(arg0);
		return;
	case SYS_VFS_PROC_GET_BY_FD:
		ctx->gpr[0] = sys_vfs_get_by_fd(arg0, (fsinfo_t*)arg1);
		return;
	case SYS_VFS_PROC_DUP2:
		ctx->gpr[0] = vfs_dup2(arg0, arg1);
		return;
	case SYS_YIELD: 
		schedule(ctx);
		return;
	case SYS_PROC_SET_CWD: 
		ctx->gpr[0] = sys_proc_set_cwd((const char*)arg0);
		return;
	case SYS_PROC_GET_CWD: 
		sys_proc_get_cwd((char*)arg0, arg1);
		return;
	}
	printf("pid:%d, code(%d) error!\n", _current_proc->pid, code);
}

