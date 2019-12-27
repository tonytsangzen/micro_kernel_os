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
#include <graph/graph.h>
#include <console.h>
#include <vfs.h>
#include <ext2fs.h>
#include <sd.h>
#include <vprintf.h>
#include <sconf.h>
#include <rawdata.h>
#include <global.h>
#include <kernel/kevent_type.h>

typedef struct {
	int fb_fd;
	int shm_id;
	int console_num;
	int cid;
	bool start_x;
	bool framebuffer;
	console_t console;
} init_t;

static void init_console(init_t* init) {
	int fb_fd = open("/dev/fb0", O_RDONLY);
	if(fb_fd < 0) {
		return;
	}

	int id = dma(fb_fd, NULL);
	if(id <= 0) {
		close(fb_fd);
		return;
	}

	void* gbuf = shm_map(id);
	if(gbuf == NULL) {
		close(fb_fd);
		return;
	}

	fbinfo_t info;
	proto_t out;
	proto_init(&out, NULL, 0);

	if(cntl_raw(fb_fd, CNTL_INFO, NULL, &out) != 0) {
		shm_unmap(id);
		close(fb_fd);
		return;
	}

	proto_read_to(&out, &info, sizeof(fbinfo_t));
	graph_t* g = graph_new(gbuf, info.width, info.height);
	proto_clear(&out);

	init->fb_fd = fb_fd;
	init->shm_id = id;
	console_init(&init->console);
	init->console.g = g;
	init->console.font = font_by_name("8x16");
	init->console.fg_color = 0xffffffff;
	init->console.bg_color = 0xff000000;
	console_reset(&init->console);
}

static void close_console(init_t* init) {
	graph_free(init->console.g);
	shm_unmap(init->shm_id);
	close(init->fb_fd);
	init->fb_fd = -1;
}

static void outc(char c, void* p) {
	char s[2];
	s[0] = c;
	s[1] = 0;

	init_t* init = (init_t*)p;
	if(init->fb_fd < 0) {
		uprintf("%s", s);
		return;
	}
	console_put_string(&init->console, s);
}
 
static void console_out(init_t* init, const char* format, ...) {
	va_list ap;
  va_start(ap, format);
  v_printf(outc, init, format, ap);
  va_end(ap);

	if(init->fb_fd >= 0) {
		flush(init->fb_fd);
	}
}

/*
static void set_keyb_table(int type) {
	syscall3(SYS_DEV_OP, DEV_KEYB, DEV_OP_SET, type);
}

static void check_keyb_table(init_t* init) {
	const char* fname = "/etc/keyb.conf";
	const char* key = "keyb_table";
	sconf_t* conf = sconf_load(fname);
	if(conf != NULL) {
		const char* kmap = sconf_get(conf, key);
		if(kmap[0] != 0) {
			set_keyb_table(kmap[0] - '0');
			sconf_free(conf);
			return;
		}
	}
	sconf_free(conf);

	int type = 0;
	console_out(init, "\n  ENTER to continue......\n");
	while(1) {
		uint8_t v;
		int rd = syscall3(SYS_DEV_CHAR_READ, (int32_t)DEV_KEYB, (int32_t)&v, 1);
		if(rd == 1) {
			if(v == 13) {	
				type = 0;
				break;
			}
			else if(v == 48 || v == 97) {
				type = 1;
				break;
			}
		}
		sleep(0);
	}
	set_keyb_table(type);

	char s[32];
	snprintf(s, 31, "%s=%d\n", key, type);
	int fd = open(fname, O_RDWR | O_CREAT);
	if(fd < 0)
		return;
	write(fd, s, strlen(s));
	close(fd);
}
*/

static void run_init_root(init_t* init, const char* cmd) {
	console_out(init, "init: mounting %32s ", "/");
	int pid = fork();
	if(pid == 0) {
		ext2_t ext2;
		ext2_init(&ext2, sd_read, sd_write);
		str_t* fname = str_new("");
		str_to(cmd, ' ', fname, 1);
		int32_t sz;
		void* data = ext2_readfile(&ext2, fname->cstr, &sz);
		str_free(fname);
		ext2_quit(&ext2);

		if(data == NULL)
			exit(-1);
		exec_elf(cmd, data, sz);
		free(data);
	}
	vfs_mount_wait("/dev", pid);
	console_out(init, "[ok]\n");
}

static void run_dev(init_t* init, const char* cmd, const char* mnt, bool prompt) {
	if(prompt)
		console_out(init, "init: mounting %32s ", mnt);

	int pid = fork();
	if(pid == 0) {
		char fcmd[FS_FULL_NAME_MAX];
		snprintf(fcmd, FS_FULL_NAME_MAX-1, "%s %s", cmd, mnt);
		exec(fcmd);
	}
	vfs_mount_wait(mnt, pid);

	if(prompt)
		console_out(init, "[ok]\n");
}

static void run(const char* cmd) {
	int pid = fork();
	if(pid == 0) {
		exec(cmd);
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
	run_dev(init, "/sbin/dev/nulld", "/dev/null", true);
	run_dev(init, "/sbin/dev/ttyd", "/dev/tty0", true);
	init_stdio();
	setenv("CONSOLE_ID", "tty");
	run("/bin/session");
}

static void init_fb(init_t* init) {
	/*mount framebuffer device*/
	run_dev(init, "/sbin/dev/fbd", "/dev/fb0", false);
	/*init framebuffer init*/
	//check_keyb_table(&init);
}
	
static void load_ui_devs(init_t* init) {
	run_dev(init, "/sbin/dev/moused", "/dev/mouse0", true);
	run_dev(init, "/sbin/dev/keybd", "/dev/keyb0", true);
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
		run(cmd);
		i++;
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	init_t init;
	memset(&init, 0, sizeof(init_t));
	init.fb_fd = -1;

	setenv("OS", "mkos");
	setenv("PATH", "/sbin:/bin");
	set_global("current_console", "0");

	console_out(&init, "\n[init process started]\n");
	/*mount root fs*/
	run_init_root(&init, "/sbin/dev/sdd");
	tty_shell(&init);

	read_conf(&init, "/etc/init.conf");
	if(init.framebuffer) {
		init_fb(&init);
		init_console(&init);
		load_ui_devs(&init);
		close_console(&init);

		console_shells(&init);
	
		if(init.start_x) {
			set_global("current_console", "x");
			run_dev(&init, "/sbin/dev/xserverd", "/dev/x", false);
			run("/bin/launcher");
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
