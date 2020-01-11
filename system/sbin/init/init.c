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
#include <kernel/kevent_type.h>
#include "sdinit.h"

typedef struct {
	int console_num;
	int cid;
	bool start_x;
	bool do_console;
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

	console_out(init, "init: load sd  %32s ", "");
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
		console_out(init, "init: mounting %32s ", mnt);

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
	run_dev(init, "/sbin/dev/fbd", "/dev/fb0", true);
	run_arch_dev(init, "ttyd", "/dev/tty0", true); 
	run_arch_dev(init, "gpiod", "/dev/gpio", true);
	run_arch_dev(init, "spid", "/dev/spi", true);
	run_arch_dev(init, "actledd", "/dev/actled", true);
	run_dev(init, "/sbin/dev/nulld", "/dev/null", true);

	init->do_console = false;
	run_arch_dev(init, "joystickd", "/dev/joystick", true);
	//run_arch_dev(init, "keybd", "/dev/keyb0", true);
	//if(run_arch_dev(init, "moused", "/dev/mouse0", true) == 0)
	//	init->do_console = true;
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

#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

#define LCD_CS   8
#define LCD_RST  27
#define LCD_DC   25
#define LCD_BL   24

#define DEV_Delay_ms(x) usleep((x)*1000)
#define DEV_Digital_Write gpio_arch_write

#define LCD_CS_0		DEV_Digital_Write(LCD_CS,0)
#define LCD_CS_1		DEV_Digital_Write(LCD_CS,1)

#define LCD_RST_0		DEV_Digital_Write(LCD_RST,0)
#define LCD_RST_1		DEV_Digital_Write(LCD_RST,1)

#define LCD_DC_0		DEV_Digital_Write(LCD_DC,0)
#define LCD_DC_1		DEV_Digital_Write(LCD_DC,1)

#define LCD_BL_0		DEV_Digital_Write(LCD_BL,0)
#define LCD_BL_1		DEV_Digital_Write(LCD_BL,1)

#define LCD_HEIGHT 240
#define LCD_WIDTH 240

#define LCD_WIDTH_Byte 240

#define HORIZONTAL 0
#define VERTICAL   1



typedef struct{
	UWORD WIDTH;
	UWORD HEIGHT;
	UBYTE SCAN_DIR;
}LCD_ATTRIBUTES;
static LCD_ATTRIBUTES LCD;

static void DEV_SPI_WriteByte(UBYTE data) {
	spi_arch_activate(1);
	spi_arch_transfer(data);
	spi_arch_activate(0);
}

static void DEV_SPI_Write_nByte(UBYTE* data, uint32_t sz) {
	spi_arch_activate(1);
	for(uint32_t i=0; i<sz; i++)
		spi_arch_transfer(data[i]);
	spi_arch_activate(0);
}

/******************************************************************************
function :	Hardware reset
parameter:
******************************************************************************/
static void LCD_Reset(void)
{
    LCD_RST_1;
    DEV_Delay_ms(100);
    LCD_RST_0;
    DEV_Delay_ms(100);
    LCD_RST_1;
    DEV_Delay_ms(100);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void LCD_SendCommand(UBYTE Reg)
{
    LCD_DC_0;
    // LCD_CS_0;
    DEV_SPI_WriteByte(Reg);
    // LCD_CS_1;
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void LCD_SendData_8Bit(UBYTE Data)
{
    LCD_DC_1;
    // LCD_CS_0;
    DEV_SPI_WriteByte(Data);
    // LCD_CS_1;
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void LCD_SendData_16Bit(UWORD Data)
{
    LCD_DC_1;
    // LCD_CS_0;
    DEV_SPI_WriteByte((Data >> 8) & 0xFF);
    DEV_SPI_WriteByte(Data & 0xFF);
    // LCD_CS_1;
}

/********************************************************************************
function:	Set the resolution and scanning method of the screen
parameter:
		Scan_dir:   Scan direction
********************************************************************************/
static void LCD_SetAttributes(UBYTE Scan_dir)
{
    //Get the screen scan direction
    LCD.SCAN_DIR = Scan_dir;
    UBYTE MemoryAccessReg = 0x00;

    //Get GRAM and LCD width and height
    if(Scan_dir == HORIZONTAL) {
        LCD.HEIGHT	= LCD_HEIGHT;
        LCD.WIDTH   = LCD_WIDTH;
        MemoryAccessReg = 0X70;
    } else {
        LCD.HEIGHT	= LCD_WIDTH;
        LCD.WIDTH   = LCD_HEIGHT;
        MemoryAccessReg = 0X00;
    }

    // Set the read / write scan direction of the frame memory
    LCD_SendCommand(0x36); //MX, MY, RGB mode
    LCD_SendData_8Bit(MemoryAccessReg);	//0x08 set RGB
}

/******************************************************************************
function :	Initialize the lcd register
parameter:
******************************************************************************/
static void LCD_InitReg(void)
{
    LCD_SendCommand(0x11); 
    DEV_Delay_ms(120);
    // LCD_SendCommand(0x36);
    // LCD_SendData_8Bit(0x00);

    LCD_SendCommand(0x3A); 
    LCD_SendData_8Bit(0x05);

    LCD_SendCommand(0xB2);
    LCD_SendData_8Bit(0x0C);
    LCD_SendData_8Bit(0x0C);
    LCD_SendData_8Bit(0x00);
    LCD_SendData_8Bit(0x33);
    LCD_SendData_8Bit(0x33); 

    LCD_SendCommand(0xB7); 
    LCD_SendData_8Bit(0x35);  

    LCD_SendCommand(0xBB);
    LCD_SendData_8Bit(0x37);

    LCD_SendCommand(0xC0);
    LCD_SendData_8Bit(0x2C);

    LCD_SendCommand(0xC2);
    LCD_SendData_8Bit(0x01);

    LCD_SendCommand(0xC3);
    LCD_SendData_8Bit(0x12);   

    LCD_SendCommand(0xC4);
    LCD_SendData_8Bit(0x20);  

    LCD_SendCommand(0xC6); 
    LCD_SendData_8Bit(0x0F);    

    LCD_SendCommand(0xD0); 
    LCD_SendData_8Bit(0xA4);
    LCD_SendData_8Bit(0xA1);

    LCD_SendCommand(0xE0);
    LCD_SendData_8Bit(0xD0);
    LCD_SendData_8Bit(0x04);
    LCD_SendData_8Bit(0x0D);
    LCD_SendData_8Bit(0x11);
    LCD_SendData_8Bit(0x13);
    LCD_SendData_8Bit(0x2B);
    LCD_SendData_8Bit(0x3F);
    LCD_SendData_8Bit(0x54);
    LCD_SendData_8Bit(0x4C);
    LCD_SendData_8Bit(0x18);
    LCD_SendData_8Bit(0x0D);
    LCD_SendData_8Bit(0x0B);
    LCD_SendData_8Bit(0x1F);
    LCD_SendData_8Bit(0x23);

    LCD_SendCommand(0xE1);
    LCD_SendData_8Bit(0xD0);
    LCD_SendData_8Bit(0x04);
    LCD_SendData_8Bit(0x0C);
    LCD_SendData_8Bit(0x11);
    LCD_SendData_8Bit(0x13);
    LCD_SendData_8Bit(0x2C);
    LCD_SendData_8Bit(0x3F);
    LCD_SendData_8Bit(0x44);
    LCD_SendData_8Bit(0x51);
    LCD_SendData_8Bit(0x2F);
    LCD_SendData_8Bit(0x1F);
    LCD_SendData_8Bit(0x1F);
    LCD_SendData_8Bit(0x20);
    LCD_SendData_8Bit(0x23);

    LCD_SendCommand(0x21); 

    LCD_SendCommand(0x29);
}

/********************************************************************************
function :	Initialize the lcd
parameter:
********************************************************************************/
void LCD_1in3_Init(UBYTE Scan_dir)
{
    //Turn on the backlight
    LCD_BL_1;

    //Hardware reset
    LCD_Reset();
kprintf("reset\n");
    //Set the resolution and scanning method of the screen
    LCD_SetAttributes(Scan_dir);
kprintf("set attr\n");
    
    //Set the initialization register
    LCD_InitReg();
kprintf("init reg\n");
}

/********************************************************************************
function:	Sets the start position and size of the display area
parameter:
		Xstart 	:   X direction Start coordinates
		Ystart  :   Y direction Start coordinates
		Xend    :   X direction end coordinates
		Yend    :   Y direction end coordinates
********************************************************************************/
void LCD_1in3_SetWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend)
{
    //set the X coordinates
    LCD_SendCommand(0x2A);
    LCD_SendData_8Bit((Xstart >> 8) & 0xFF);
    LCD_SendData_8Bit(Xstart & 0xFF);
    LCD_SendData_8Bit(((Xend  - 1) >> 8) & 0xFF);
    LCD_SendData_8Bit((Xend  - 1) & 0xFF);

    //set the Y coordinates
    LCD_SendCommand(0x2B);
    LCD_SendData_8Bit((Ystart >> 8) & 0xFF);
    LCD_SendData_8Bit(Ystart & 0xFF);
    LCD_SendData_8Bit(((Yend  - 1) >> 8) & 0xFF);
    LCD_SendData_8Bit((Yend  - 1) & 0xFF);

    LCD_SendCommand(0X2C);
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void LCD_1in3_Clear(UWORD Color)
{
    UWORD j;
    UWORD Image[LCD_WIDTH*LCD_HEIGHT];
    
    Color = ((Color<<8)&0xff00)|(Color>>8);
   
    for (j = 0; j < LCD_HEIGHT*LCD_WIDTH; j++) {
        Image[j] = Color;
    }
    
    LCD_1in3_SetWindows(0, 0, LCD_WIDTH, LCD_HEIGHT);
    LCD_DC_1;
    for(j = 0; j < LCD_HEIGHT; j++){
        DEV_SPI_Write_nByte((uint8_t *)&Image[j*LCD_WIDTH], LCD_WIDTH*2);
    }
}


void test(void) {
	gpio_arch_init();

	gpio_arch_config(LCD_CS, 1);
	gpio_arch_config(LCD_RST, 1);
	gpio_arch_config(LCD_DC, 1);
	gpio_arch_config(LCD_BL, 1);

	spi_arch_init(1024);
	spi_arch_select(1);

	LCD_1in3_Init(HORIZONTAL);
kprintf("inited\n");
	LCD_1in3_Clear(0xFFFF);
kprintf("cleared\n");
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

	load_devs(&init);
	tty_shell(&init);
	read_conf(&init, "/etc/init.conf");
	if(init.do_console) {
		console_shells(&init);
		if(init.start_x) {
			set_global("current_console", "x");
			run_dev(&init, "/sbin/dev/xserverd", "/dev/x", false);
			run(&init, "/bin/launcher");
		}
	}
	else {
		console_out(&init, "\ninput devices load failed, only do tty shell!\n");
	}

	test();
	int fd = open("/dev/joystick", O_RDONLY);
	while(1) {
		//kevent_handle(&init);
		char c;
		read(fd, &c, 1);
		if(c != 0)
			kprintf("key: 0x%x\n", c);
	}
	close(fd);
	return 0;
}
