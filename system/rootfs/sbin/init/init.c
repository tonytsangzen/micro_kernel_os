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

static void init_stdio(void) {
	int fd = open("/dev/tty0", 0);
	dup2(fd, 0);
	dup2(fd, 1);
}

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

static void console_out(init_console_t* console, const char* s) {
	console_put_string(&console->console, s);
	flush(console->fb_fd);
}

static void check_keyb_table(void) {
	while(1) {
		uint8_t v;
		int rd = syscall3(SYS_DEV_READ, (int32_t)DEV_KEYB, (int32_t)&v, 1);
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

static void run_init_dev(const char* cmd, const char* dev, uint8_t fs_ready) {
	int pid = fork();
	if(pid == 0) {
		if(fs_ready != 0)
			exec(cmd);
		else
			exec_initfs(cmd);
	}
	vfs_mount_wait(dev, pid);
	uprintf("%s mounted (%s).\n", dev, cmd);
}

static void run_dev(init_console_t* console, const char* cmd, const char* dev) {
	int pid = fork();
	if(pid == 0) {
		exec(cmd);
	}
	vfs_mount_wait(dev, pid);
	console_out(console, dev);
	console_out(console, " mounted (");
	console_out(console, cmd);
	console_out(console, ").\n");
}

static void run(init_console_t* console, const char* cmd) {
	(void)console;
	int pid = fork();
	if(pid == 0) {
		exec(cmd);
	}
}

static void console_welcome(init_console_t* console) {
	const char* s = ""
			"+-----Ewok micro-kernel OS-----------------------+\n"
			"| https://github.com/MisaZhu/micro_kernel_os.git |\n"
			"+------------------------------------------------+\n"
			"/ mounted (sbin/dev/initrd).\n"
			"/dev/fb0 mounted (sbin/dev/fbd).\n";
	console_out(console, s);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	setenv("OS", "mkos");
	setenv("PATH", "/sbin:/bin");
	uprintf("\n[init process started]\n");

	run_init_dev("sbin/dev/initrd", "/dev", 0);
	run_init_dev("/sbin/dev/fbd", "/dev/fb0", 1);

	init_console_t console;
	init_console(&console);
	uprintf("init console ready.\n");

	console_welcome(&console);

	run_dev(&console, "/sbin/dev/ttyd", "/dev/tty0");
	run_dev(&console, "/sbin/dev/nulld", "/dev/null");
	run_dev(&console, "/sbin/dev/keybd", "/dev/keyb0");
	run_dev(&console, "/sbin/dev/moused", "/dev/mouse0");

	console_out(&console, "\nEnter to continue......\n");
	check_keyb_table();

	run_dev(&console, "/sbin/dev/xserverd", "/dev/x");
	run(&console, "/bin/launcher");

	init_stdio();
	run(&console, "/bin/shell");

	while(1) sleep(1);
	close_console(&console);
	return 0;
}
