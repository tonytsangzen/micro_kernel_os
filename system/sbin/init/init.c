#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <fcntl.h>
#include <dev/fbinfo.h>
#include <syscall.h>
#include <dev/device.h>
#include <shm.h>
#include <vfs.h>
#include <ext2fs.h>
#include <vprintf.h>
#include <sysinfo.h>
#include <sconf.h>
#include <sd.h>
#include <rawdata.h>
#include <global.h>
#include <kernel/kevent_type.h>

typedef struct {
	int console_num;
	int cid;
	bool start_x;
	bool framebuffer;
} init_t;


static void outc(char c, void* p) {
	str_t* buf = (str_t*)p;
	str_addc(buf, c);
}

static void console_out(init_t* init, const char* format, ...) {
	(void)init;
	va_list ap;
	va_start(ap, format);
	str_t* buf = str_new("");
	v_printf(outc, buf, format, ap);
	va_end(ap);
	kprintf("%s", buf->cstr);
	str_free(buf);
}

static void run_init_root(init_t* init, const char* cmd) {
	console_out(init, "init: mounting %32s ", "/");
	int pid = fork();
	if(pid == 0) {
		sd_init();
		ext2_t ext2;
		ext2_init(&ext2, sd_read, sd_write);
		str_t* fname = str_new("");
		str_to(cmd, ' ', fname, 1);
		int32_t sz;
		void* data = ext2_readfile(&ext2, fname->cstr, &sz);
		str_free(fname);
		ext2_quit(&ext2);

		if(data == NULL) {
			console_out(init, "[error!]\n");
			exit(-1);
		}
		exec_elf(cmd, data, sz);
		free(data);
	}
	vfs_mount_wait("/dev", pid);
	console_out(init, "[ok]\n");
}

static int run_dev(init_t* init, const char* cmd, const char* mnt, bool prompt) {
	if(prompt)
		console_out(init, "init: mounting %32s ", mnt);

	int pid = fork();
	if(pid == 0) {
		char fcmd[FS_FULL_NAME_MAX];
		snprintf(fcmd, FS_FULL_NAME_MAX-1, "%s %s", cmd, mnt);
		if(exec(fcmd) != 0) {
			if(prompt)
				console_out(init, "[error!]\n");
			return -1;
		}
	}
	vfs_mount_wait(mnt, pid);

	if(prompt)
		console_out(init, "[ok]\n");
	return 0;
}
	
static int run_arch_dev(init_t* init, const char* dev, const char* mnt) {
	sysinfo_t sysinfo;
	syscall1(SYS_GET_SYSINFO, (int32_t)&sysinfo);
	char devfn[FS_FULL_NAME_MAX];
	snprintf(devfn, FS_FULL_NAME_MAX-1, "/sbin/dev/%s/%s", sysinfo.machine, dev);
	return run_dev(init, devfn, mnt, true);
}

static void run(init_t* init, const char* cmd) {
	int pid = fork();
	if(pid == 0) {
		if(exec(cmd) != 0)
			console_out(init, "init: run %38s [error!]", cmd);
	}
}

static void init_stdio(void) {
	int fd = open("/dev/tty0", 0);
	dup2(fd, 0);
	dup2(fd, 1);
}

static void kevent_handle(init_t* init, int32_t type, rawdata_t* data) {
	(void)data;
	if(type == KEV_CONSOLE_SWITCH) {
		char id[2];
		const char* s = get_global("current_console");
		if(s[0] == 'x') {
			set_global("current_console", "0");
			init->cid = 0;
		}
		else {
			init->cid++;
			if(init->cid >= init->console_num) {
				if(init->start_x)
					set_global("current_console", "x");
				else
					set_global("current_console", "0");
				init->cid = 0;
			}
			else {
				id[1] = 0;
				id[0] = '0' + init->cid;
				set_global("current_console", id);
			}
		}
	}
}

static int32_t read_conf(init_t* init, const char* fname) {
	init->console_num = 2;
	init->start_x = false;
	init->framebuffer = false;

	sconf_t *sconf = sconf_load(fname);	
	if(sconf == NULL)
		return -1;

	const char* v = sconf_get(sconf, "console_num");
	if(v[0] != 0) 
		init->console_num = atoi(v);
	v = sconf_get(sconf, "start_x");
	if(v[0] != 0) 
		init->start_x = str_to_bool(v);
	v = sconf_get(sconf, "framebuffer");
	if(v[0] != 0) 
		init->framebuffer = str_to_bool(v);
	sconf_free(sconf);
	return 0;
}

static void tty_shell(init_t* init) {
	/*run tty shell*/
	init_stdio();
	setenv("CONSOLE_ID", "tty");
	run(init, "/bin/session");
}
	
static void load_devs(init_t* init) {
	run_dev(init, "/sbin/dev/nulld", "/dev/null", true);
	run_dev(init, "/sbin/dev/fbd", "/dev/fb0", true);

	run_arch_dev(init, "ttyd", "/dev/tty0");
	run_arch_dev(init, "moused", "/dev/mouse0");
	run_arch_dev(init, "keybd", "/dev/keyb0");
}

static void console_shells(init_t* init) {
	/*run screen init shell*/
	int i = 0;
	while(i < init->console_num) {
		char cmd[64];
		char cid[16];
		snprintf(cid, 15, "console-%d/%d", i+1, init->console_num);
		setenv("CONSOLE_ID", cid);
		snprintf(cmd, 64, "/bin/console %d", i);
		run(init, cmd);
		i++;
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	init_t init;
	memset(&init, 0, sizeof(init_t));

	setenv("OS", "mkos");
	setenv("PATH", "/sbin:/bin");
	set_global("current_console", "0");

	console_out(&init, "\n[init process started]\n");
	//mount root fs
	run_init_root(&init, "/sbin/dev/sdd");

	load_devs(&init);
	tty_shell(&init);
	read_conf(&init, "/etc/init.conf");
	if(init.framebuffer) {
		console_shells(&init);
		if(init.start_x) {
			set_global("current_console", "x");
			run_dev(&init, "/sbin/dev/xserverd", "/dev/x", false);
			run(&init, "/bin/launcher");
		}
	}

	while(1) {
		int32_t type;
		rawdata_t data;
		if(syscall2(SYS_GET_KEVENT, (int32_t)&type, (int32_t)&data) != 0) {
			continue;
		}
		kevent_handle(&init, type, &data);
	}
	return 0;
}
