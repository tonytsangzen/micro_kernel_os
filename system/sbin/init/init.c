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
#include <gpio.h>
#include <rawdata.h>
#include <global.h>
#include <graph/graph.h>
#include <kernel/kevent_type.h>
#include <console.h>
#include "sdinit.h"

typedef struct {
	int console_num;
	int cid;
	bool start_x;
	bool do_console;
	bool do_joystick_shell;
	console_t promt_console;
	int promt_fd;
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
	if(init->promt_console.g != NULL) {
		console_put_string(&init->promt_console, buf->cstr);
		write(init->promt_fd, init->promt_console.g->buffer, init->promt_console.g->w*init->promt_console.g->h*4);
	}
	str_free(buf);
}

static inline void wait_ready(int pid) {
	while(1) {
		if(dev_ping(pid) == 0)
			break;
		sleep(1);
	}
}

static void run_init_sd(init_t* init, const char* cmd) {
	sysinfo_t sysinfo;
	syscall1(SYS_GET_SYSINFO, (int32_t)&sysinfo);
	char devfn[FS_FULL_NAME_MAX];
	snprintf(devfn, FS_FULL_NAME_MAX-1, "/sbin/dev/%s/%s", sysinfo.machine, cmd);

	console_out(init, "init: load sd %8s ", "");
	int pid = fork();
	if(pid == 0) {
		sdinit_init();
		ext2_t ext2;
		ext2_init(&ext2, sdinit_read, NULL);
		int32_t sz;
		void* data = ext2_readfile(&ext2, devfn, &sz);
		ext2_quit(&ext2);

		if(data == NULL) {
			console_out(init, "[error!] (%s)\n", devfn);
			exit(-1);
		}
		exec_elf(devfn, data, sz);
		free(data);
	}
	wait_ready(pid);
	console_out(init, "[ok]\n");
}

static void run_init_root(init_t* init, const char* cmd) {
	console_out(init, "init: %16s ", "/");
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
			console_out(init, "[error!] (%s)\n", cmd);
			exit(-1);
		}
		exec_elf(cmd, data, sz);
		free(data);
	}
	wait_ready(pid);
	console_out(init, "[ok]\n");
}

static int run_dev(init_t* init, const char* cmd, const char* mnt, bool prompt) {
	if(prompt)
		console_out(init, "init: %16s ", mnt);

	if(vfs_access(cmd) == 0) {
		int pid = fork();
		if(pid == 0) {
			char fcmd[FS_FULL_NAME_MAX];
			snprintf(fcmd, FS_FULL_NAME_MAX-1, "%s %s", cmd, mnt);
			if(exec(fcmd) != 0) {
				if(prompt)
					console_out(init, "[error!] (%s)\n", cmd);
				exit(-1);
			}
		}
		wait_ready(pid);
	}
	else {
		if(prompt)
			console_out(init, "[failed!](no such file)\n");
		return -1;
	}

	if(prompt)
		console_out(init, "[ok]\n");
	return 0;
}
	
static int run_arch_dev(init_t* init, const char* dev, const char* mnt, bool prompt) {
	sysinfo_t sysinfo;
	syscall1(SYS_GET_SYSINFO, (int32_t)&sysinfo);
	char devfn[FS_FULL_NAME_MAX];
	snprintf(devfn, FS_FULL_NAME_MAX-1, "/sbin/dev/%s/%s", sysinfo.machine, dev);
	return run_dev(init, devfn, mnt, prompt);
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

static void promt_init(init_t* init) {
	int fd = -1;
	if(run_arch_dev(init, "lcdd", "/dev/fb1", true) == 0)
		fd = open("/dev/fb1", O_RDWR);

	if(fd < 0) 
		fd = open("/dev/fb0", O_RDWR);
	if(fd < 0) 
		return;

	proto_t out;	
	proto_init(&out, NULL, 0);
	int res = fcntl_raw(fd, CNTL_INFO, NULL, &out);
	if(res != 0) {
		close(fd);
		return;
	}
	int w = proto_read_int(&out);
	int h = proto_read_int(&out);
	proto_clear(&out);
	
	init->promt_fd = fd;
	console_init(&init->promt_console);
	graph_t* g = graph_new(NULL, w, h);
	init->promt_console.g = g;
	init->promt_console.font = font_by_name("8x16");
	console_reset(&init->promt_console);
}

static void promt_close(init_t* init) {
	if(init->promt_console.g == NULL)
		return;
	graph_free(init->promt_console.g);
	console_close(&init->promt_console);
	close(init->promt_fd);
}

static int32_t read_conf(init_t* init, const char* fname) {
	init->console_num = 2;
	init->start_x = false;

	sconf_t *sconf = sconf_load(fname);	
	if(sconf == NULL)
		return -1;

	const char* v = sconf_get(sconf, "console_num");
	if(v[0] != 0) 
		init->console_num = atoi(v);
	v = sconf_get(sconf, "start_x");
	if(v[0] != 0) 
		init->start_x = str_to_bool(v);
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
	run_arch_dev(init, "ttyd", "/dev/tty0", true); 
	run_arch_dev(init, "gpiod", "/dev/gpio", true);
	run_arch_dev(init, "actledd", "/dev/actled", true);
	run_arch_dev(init, "spid", "/dev/spi", true);

	if(run_arch_dev(init, "joystickd", "/dev/joystick", true) == 0)
		init->do_joystick_shell = true;

	run_arch_dev(init, "keybd", "/dev/keyb0", true);
	if(run_arch_dev(init, "moused", "/dev/mouse0", true) == 0)
		init->do_console = true;
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

static void kevent_handle(init_t* init) {
	int32_t type;
	rawdata_t data;
	if(syscall2(SYS_GET_KEVENT, (int32_t)&type, (int32_t)&data) != 0) {
		return;
	}

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
	run_init_sd(&init, "sdd");
	run_init_root(&init, "/sbin/dev/rootfsd");
	run_dev(&init, "/sbin/dev/fbd", "/dev/fb0", true);

	promt_init(&init);
	load_devs(&init);
	tty_shell(&init);

	read_conf(&init, "/etc/init.conf");
	if(init.do_console) {
		console_shells(&init);
		if(init.start_x) {
			set_global("current_console", "x");
			run_dev(&init, "/sbin/dev/xserverd", "/dev/x", true);
			run(&init, "/bin/launcher");
		}
	}

	promt_close(&init);

	while(1) {
		kevent_handle(&init);
	}
	return 0;
}
