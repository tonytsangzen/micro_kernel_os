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

typedef struct {
	graph_t* g;
	int fb_fd;
	int shm_id;
	console_t console;
} init_console_t;

static void init_console(init_console_t* console) {
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

	console->fb_fd = fb_fd;
	console->shm_id = id;
	console_init(&console->console);
	console->console.g = g;
	console->console.font = font_by_name("8x16");
	console->console.fg_color = 0xffffffff;
	console->console.bg_color = 0xff000000;
	console_reset(&console->console);
}

static void close_console(init_console_t* console) {
	graph_free(console->g);
	shm_unmap(console->shm_id);
	close(console->fb_fd);
}

static void outc(char c, void* p) {
	char s[2];
	s[0] = c;
	s[1] = 0;

	init_console_t* console = (init_console_t*)p;
	if(console->fb_fd < 0) {
		uprintf("%s", s);
		return;
	}
	console_put_string(&console->console, s);
}
 
static void console_out(init_console_t* console, const char* format, ...) {
	va_list ap;
  va_start(ap, format);
  v_printf(outc, console, format, ap);
  va_end(ap);
	flush(console->fb_fd);
}

static void check_keyb_table(init_console_t* console) {
	console_out(console, "  ENTER to continue......\n");
	while(1) {
		uint8_t v;
		int rd = syscall3(SYS_DEV_CHAR_READ, (int32_t)DEV_KEYB, (int32_t)&v, 1);
		if(rd != 1)
			continue;

		if(v == 13) {	
			syscall3(SYS_DEV_OP, DEV_KEYB, DEV_OP_SET, 0);
			break;
		}
		else if(v == 48 || v == 97) {
			syscall3(SYS_DEV_OP, DEV_KEYB, DEV_OP_SET, 1);
			break;
		}
	}
}

static void run_init_root(init_console_t* console, const char* cmd) {
	console_out(console, "init: mounting %32s ", "/");
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
	console_out(console, "[ok]\n");
}

static void run_dev(init_console_t* console, const char* cmd, const char* mnt) {
	console_out(console, "init: mounting %32s ", mnt);
	int pid = fork();
	if(pid == 0) {
		char fcmd[FS_FULL_NAME_MAX];
		snprintf(fcmd, FS_FULL_NAME_MAX-1, "%s %s", cmd, mnt);
		exec(fcmd);
	}
	vfs_mount_wait(mnt, pid);
	console_out(console, "[ok]\n");
}

static void run(const char* cmd) {
	int pid = fork();
	if(pid == 0) {
		exec(cmd);
	}
}

static void welcome(init_console_t* console) {
	const char* s = ""
			"+-----Ewok micro-kernel OS-----------------------+\n"
			"| https://github.com/MisaZhu/micro_kernel_os.git |\n"
			"+------------------------------------------------+\n";
	console_out(console, s);
}

static void init_stdio(void) {
	int fd = open("/dev/tty0", 0);
	dup2(fd, 0);
	dup2(fd, 1);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	init_console_t console;
	console.fb_fd = -1;

	setenv("OS", "mkos");
	setenv("PATH", "/sbin:/bin");

	console_out(&console, "\n[init process started]\n");
	run_init_root(&console, "/sbin/dev/sdd");

	run_dev(&console, "/sbin/dev/nulld", "/dev/null");
	run_dev(&console, "/sbin/dev/ttyd", "/dev/tty0");
	run_dev(&console, "/sbin/dev/fbd", "/dev/fb0");
	init_console(&console);
	welcome(&console);
	
	init_stdio();
	run("/bin/shell");

	run_dev(&console, "/sbin/dev/moused", "/dev/mouse0");
	run_dev(&console, "/sbin/dev/keybd", "/dev/keyb0");
	check_keyb_table(&console);

	run_dev(&console, "/sbin/dev/xserverd", "/dev/x");
	run("/bin/launcher");
	close_console(&console);

	while(1) sleep(1);
	return 0;
}
