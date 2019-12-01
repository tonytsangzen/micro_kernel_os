#include <kernel/kernel.h>
#include <kernel/svc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <kernel/proc.h>
#include <kernel/hw_info.h>
#include <mm/kalloc.h>
#include <mm/shm.h>
#include <mm/kmalloc.h>
#include <sysinfo.h>
#include <vfs.h>
#include <syscalls.h>
#include <kstring.h>
#include <kprintf.h>
#include <buffer.h>
#include <dev/kdevice.h>
#include <dev/framebuffer.h>

static void sys_exit(context_t* ctx, int32_t res) {
	if(_current_proc == NULL)
		return;
	proc_exit(ctx, _current_proc, res);
}

static int32_t sys_dev_block_read(uint32_t type, int32_t bid) {
	dev_t* dev = get_dev(type);
	if(dev == NULL) {
		return -1;
	}
	return dev_block_read(dev, bid);
}

static int32_t sys_dev_block_write(uint32_t type, int32_t bid, const char* buf) {
	dev_t* dev = get_dev(type);
	if(dev == NULL) {
		return -1;
	}
	return dev_block_write(dev, bid, buf);
}

static void sys_dev_block_read_done(context_t* ctx, uint32_t type, void* buf) {
	dev_t* dev = get_dev(type);
	if(dev == NULL) {
		ctx->gpr[0] = -1;
		return;
	}		

	int res = dev_block_read_done(dev, buf);
	if(res == 0) {
		ctx->gpr[0] = res;
		return;
	}

	proc_t* proc = _current_proc;
	proc_sleep_on(ctx, (uint32_t)&proc->pid);
	proc->ctx.gpr[0] = -1;
}

static void sys_dev_block_write_done(context_t* ctx, uint32_t type) {
	dev_t* dev = get_dev(type);
	if(dev == NULL) {
		ctx->gpr[0] = -1;
		return;
	}		

	int res = dev_block_write_done(dev);
	if(res == 0) {
		ctx->gpr[0] = res;
		return;
	}

	proc_t* proc = _current_proc;
	proc_sleep_on(ctx, (uint32_t)&proc->pid);
	proc->ctx.gpr[0] = -1;
}

static int32_t sys_dev_op(uint32_t type, int32_t opcode, int32_t arg) {
	dev_t* dev = get_dev(type);
	if(dev == NULL)
		return -1;
	return dev_op(dev, opcode, arg);
}

static void sys_dev_ch_read(context_t* ctx, uint32_t type, void* data, uint32_t sz) {
	dev_t* dev = get_dev(type);
	if(dev == NULL) {
		ctx->gpr[0] = -1;
		return;
	}

	int32_t rd = dev_ch_read(dev, data, sz);
	ctx->gpr[0] = rd;

	if(rd != DEV_SLEEP) //not sleep for
		return;
	proc_t* proc = _current_proc;
	proc_sleep_on(ctx, (uint32_t)dev);
	proc->ctx.gpr[0] = 0;
}

static int32_t sys_dev_ch_write(uint32_t type, void* data, uint32_t sz) {
	dev_t* dev = get_dev(type);
	if(dev == NULL)
		return -1;
	return dev_ch_write(dev, data, sz);
}

static int32_t sys_getpid(void) {
	if(_current_proc == NULL)
		return -1;
	return _current_proc->pid;
}

static void sys_sleep_on(context_t* ctx, uint32_t event) {
	proc_sleep_on(ctx, event);
}

static void sys_sleep(context_t* ctx, uint32_t count) {
	proc_sleep(ctx, count);
}

static void sys_wakeup(uint32_t event) {
	proc_wakeup(event);
}

static void sys_kill(context_t* ctx, int32_t pid) {
	proc_t* proc = proc_get(pid);
	if(proc == NULL)
		return;

	if(_current_proc->owner == 0 || proc->owner == _current_proc->owner) {
		proc_exit(ctx, proc, 0);
	}
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
	if(node_to == NULL)
		return -1;
	vfs_node_t* node = vfs_get(node_to, info->name);
	if(node != NULL) {
		get_fsinfo(node, info);
		return 0;
	}

	node = (vfs_node_t*)info->node;
	if(node == NULL)
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

static int32_t sys_vfs_get_mount_by_id(int32_t id, mount_t* mount) {
	if(id < 0 || mount == NULL)
		return -1;
	return vfs_get_mount_by_id(id, mount);
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
	
	str_cpy(_current_proc->cmd, cmd);
	if(proc_load_elf(_current_proc, elf, elf_size) != 0)
		return -1;

	memcpy(ctx, &_current_proc->ctx, sizeof(context_t));
	return 0;
}

static void sys_get_msg(context_t* ctx, int32_t *pid, rawdata_t* data, int32_t id) {
	int32_t res = proc_get_msg(pid, data, id);
	if(res >= 0) {
		ctx->gpr[0] = res;
		return;
	}

	proc_t* proc = _current_proc;
	proc_sleep_on(ctx, (uint32_t)&proc->pid);
	proc->ctx.gpr[0] = -1;
}

static int32_t sys_proc_set_cwd(const char* cwd) {
	str_cpy(_current_proc->cwd, cwd);
	return 0;
}

static void sys_proc_get_cwd(char* cwd, int32_t sz) {
	strncpy(cwd, CS(_current_proc->cwd), sz);
}

static void sys_proc_get_cmd(char* cmd, int32_t sz) {
	strncpy(cmd, CS(_current_proc->cmd), sz);
}

static void	sys_get_sysinfo(sysinfo_t* info) {
	if(info == NULL)
		return;

	info->free_mem = get_free_mem_size();
	info->total_mem = _hw_info.phy_mem_size;
	info->shm_mem = shm_alloced_size();
	info->kernel_tic = _kernel_tic;
}

static int32_t sys_pipe_open(int32_t* fd0, int32_t* fd1) {
	if(fd0 == NULL || fd1 == NULL)
		return -1;

	vfs_node_t* node = vfs_new_node();
	if(node == NULL)
		return -1;
	node->fsinfo.type = FS_TYPE_PIPE;

	int fd = vfs_open(_current_proc->pid, node, 1);
	if(fd < 0) {
		kfree(node);
		return -1;
	}
	*fd0 = fd;
	
	fd = vfs_open(_current_proc->pid, node, 1);
	if(fd < 0) {
		vfs_close(_current_proc, *fd0);
		kfree(node);
		return -1;
	}
	*fd1 = fd;

	buffer_t* buf = (buffer_t*)kmalloc(sizeof(buffer_t));
	memset(buf, 0, sizeof(buffer_t));
	node->fsinfo.data = (int32_t)buf;
	return 0;
}

static int32_t sys_pipe_write(fsinfo_t* info, const void* data, uint32_t sz) {
	if(info == NULL || info->type != FS_TYPE_PIPE)
		return -1;

	buffer_t* buffer = (buffer_t*)info->data;
	if(buffer == NULL)
		return -1;

	int32_t res = buffer_write(buffer, data, sz);
	if(res > 0)
		return res;

	vfs_node_t* node = (vfs_node_t*)info->node;
	if(node->refs >= 2)
		return 0;
	return -1; //closed
}

static int32_t sys_pipe_read(fsinfo_t* info, void* data, uint32_t sz) {
	if(info == NULL || info->type != FS_TYPE_PIPE)
		return -1;

	buffer_t* buffer = (buffer_t*)info->data;
	if(buffer == NULL)
		return -1;

	int32_t res =  buffer_read(buffer, data, sz);
	if(res > 0)
		return res;

	vfs_node_t* node = (vfs_node_t*)info->node;
	if(node->refs >= 2)
		return 0;
	return -1; //closed.
}

static int32_t sys_get_env(const char* name, char* value, int32_t size) {
	const char* v = proc_get_env(name);
	if(v == NULL)
		value[0] = 0;
	else
		strncpy(value, v, size);
	return 0;
}

static int32_t sys_get_env_name(int32_t index, char* name, int32_t size) {
	const char* n = proc_get_env_name(index);
	if(n[0] == 0)
		return -1;
	strncpy(name, n, size);
	return 0;
}

static int32_t sys_get_env_value(int32_t index, char* value, int32_t size) {
	const char* v = proc_get_env_value(index);
	if(v == NULL)
		value[0] = 0;
	else
		strncpy(value, v, size);
	return 0;
}

static int32_t sys_shm_alloc(uint32_t size, int32_t flag) {
	return shm_alloc(size, flag);
}

static void* sys_shm_map(int32_t id) {
	return shm_proc_map(_current_proc->pid, id);
}

static int32_t sys_shm_unmap(int32_t id) {
	return shm_proc_unmap(_current_proc->pid, id);
}

static int32_t sys_shm_ref(int32_t id) {
	return shm_proc_ref(_current_proc->pid, id);
}

static int32_t sys_framebuffer(void) {
	if(_current_proc->owner != 0)
		return -1;

  uint32_t fb_base = (uint32_t)V2P(_framebuffer_base); //framebuffer addr
  uint32_t fb_end = (uint32_t)V2P(_framebuffer_end); //framebuffer addr
  map_pages(_current_proc->space->vm, fb_base, fb_base, fb_end, AP_RW_RW);
	return fb_base;
}

static int32_t sys_send_msg(int32_t topid, rawdata_t* data, int32_t id) {
	proc_msg_t* msg = proc_send_msg(topid, data, id);
	if(msg == NULL)
		return -1;
	return msg->id;	
}

static int32_t sys_initrd(void) {
#ifdef _INITRD
	return (int32_t)_initrd;
#else
	return 0;
#endif
}

void svc_handler(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2, context_t* ctx, int32_t processor_mode) {
	(void)arg1;
	(void)arg2;
	(void)ctx;
	(void)processor_mode;
	
	__irq_disable();

	switch(code) {
	case SYS_DEV_CHAR_READ:
		sys_dev_ch_read(ctx, arg0, (void*)arg1, arg2);		
		return;
	case SYS_DEV_CHAR_WRITE:
		ctx->gpr[0] = sys_dev_ch_write(arg0, (void*)arg1, arg2);		
		return;
	case SYS_DEV_BLOCK_READ:
		ctx->gpr[0] = sys_dev_block_read(arg0, arg1);
		return;
	case SYS_DEV_BLOCK_WRITE:
		ctx->gpr[0] = sys_dev_block_write(arg0, arg1, (void*)arg2);		
		return;
	case SYS_DEV_BLOCK_READ_DONE:
		sys_dev_block_read_done(ctx, arg0, (void*)arg1);
		return;
	case SYS_DEV_BLOCK_WRITE_DONE:
		sys_dev_block_write_done(ctx, arg0);
		return;
	case SYS_DEV_OP:
		ctx->gpr[0] = sys_dev_op(arg0, arg1, arg2);
		return;
	case SYS_INITRD:
		ctx->gpr[0] = sys_initrd();
		return;
	case SYS_FRAMEBUFFER:
		ctx->gpr[0] = (int32_t)sys_framebuffer();
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
	case SYS_SLEEP:
		sys_sleep(ctx, (uint32_t)arg0);
		return;
	case SYS_WAKEUP:
		sys_wakeup((uint32_t)arg0);
		return;
	case SYS_KILL:
		sys_kill(ctx, arg0);
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
		ctx->gpr[0] = sys_send_msg(arg0, (rawdata_t*)arg1, arg2);
		return;
	case SYS_GET_MSG_NBLOCK:
		ctx->gpr[0] = proc_get_msg((int32_t*)arg0, (rawdata_t*)arg1, arg2);
		return;
	case SYS_GET_MSG:
		sys_get_msg(ctx, (int32_t*)arg0, (rawdata_t*)arg1, arg2);
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
	case SYS_VFS_GET_MOUNT_BY_ID:
		ctx->gpr[0] = sys_vfs_get_mount_by_id(arg0, (mount_t*)arg1);
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
	case SYS_VFS_PROC_DUP:
		ctx->gpr[0] = vfs_dup(arg0);
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
	case SYS_PROC_GET_CMD: 
		sys_proc_get_cmd((char*)arg0, arg1);
		return;
	case SYS_GET_SYSINFO:
		sys_get_sysinfo((sysinfo_t*)arg0);
		return;
	case SYS_GET_PROCS: 
		ctx->gpr[0] = (int32_t)get_procs((int32_t*)arg0);
		return;
	case SYS_PIPE_OPEN: 
		ctx->gpr[0] = sys_pipe_open((int32_t*)arg0, (int32_t*)arg1);
		return;
	case SYS_PIPE_READ: 
		ctx->gpr[0] = sys_pipe_read((fsinfo_t*)arg0, (void*)arg1, (int32_t)arg2);
		return;
	case SYS_PIPE_WRITE: 
		ctx->gpr[0] = sys_pipe_write((fsinfo_t*)arg0, (const void*)arg1, (int32_t)arg2);
		return;
	case SYS_PROC_SET_ENV: 
		ctx->gpr[0] = proc_set_env((const char*)arg0, (const char*)arg1);
		return;
	case SYS_PROC_GET_ENV: 
		ctx->gpr[0] = sys_get_env((const char*)arg0, (char*)arg1, arg2);
		return;
	case SYS_PROC_GET_ENV_NAME: 
		ctx->gpr[0] = sys_get_env_name(arg0, (char*)arg1, arg2);
		return;
	case SYS_PROC_GET_ENV_VALUE: 
		ctx->gpr[0] = sys_get_env_value(arg0, (char*)arg1, arg2);
		return;
	case SYS_PROC_SHM_ALLOC:
		ctx->gpr[0] = sys_shm_alloc(arg0, arg1);
		return;
	case SYS_PROC_SHM_MAP:
		ctx->gpr[0] = (int32_t)sys_shm_map(arg0);
		return;
	case SYS_PROC_SHM_UNMAP:
		ctx->gpr[0] = sys_shm_unmap(arg0);
		return;
	case SYS_PROC_SHM_REF:
		ctx->gpr[0] = sys_shm_ref(arg0);
		return;
	}
	printf("pid:%d, code(%d) error!\n", _current_proc->pid, code);
}

