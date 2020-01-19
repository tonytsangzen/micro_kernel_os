#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmain.h>
#include <string.h>
#include <vfs.h>
#include <gpio.h>
#include <vdevice.h>
#include <syscall.h>
#include <dev/device.h>

static int _gpio_fd = -1;

#define KEY_UP_PIN      6
#define KEY_DOWN_PIN    19
#define KEY_LEFT_PIN    5
#define KEY_RIGHT_PIN   26
#define KEY_PRESS_PIN   13
#define KEY1_PIN        21
#define KEY2_PIN        20
#define KEY3_PIN        16

#define KEY_V_UP        0x1
#define KEY_V_DOWN      0x2
#define KEY_V_LEFT      0x4
#define KEY_V_RIGHT     0x8
#define KEY_V_PRESS     0x10
#define KEY_V_1         0x20
#define KEY_V_2         0x40
#define KEY_V_3         0x80

static int joystick_read(int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)size;
	(void)p;

	char* rd = (char*)buf;
	*rd = 0;

	if(gpio_read(_gpio_fd, KEY_UP_PIN) == 0)
		*rd |= KEY_V_UP;
	if(gpio_read(_gpio_fd, KEY_DOWN_PIN) == 0)
		*rd |= KEY_V_DOWN;
	if(gpio_read(_gpio_fd, KEY_LEFT_PIN) == 0)
		*rd |= KEY_V_LEFT;
	if(gpio_read(_gpio_fd, KEY_RIGHT_PIN) == 0)
		*rd |= KEY_V_RIGHT;
	if(gpio_read(_gpio_fd, KEY_PRESS_PIN) == 0)
		*rd |= KEY_V_PRESS;
	if(gpio_read(_gpio_fd, KEY1_PIN) == 0)
		*rd |= KEY_V_1;
	if(gpio_read(_gpio_fd, KEY2_PIN) == 0)
		*rd |= KEY_V_2;
	if(gpio_read(_gpio_fd, KEY3_PIN) == 0)
		*rd |= KEY_V_3;
	return 1;
}

static void init_gpio(void) {
	gpio_config(_gpio_fd, KEY_UP_PIN, 0);//input	
	gpio_pull(_gpio_fd, KEY_UP_PIN, 2); //pull up

	gpio_config(_gpio_fd, KEY_DOWN_PIN, 0);//input	
	gpio_pull(_gpio_fd, KEY_DOWN_PIN, 2); //pull up

	gpio_config(_gpio_fd, KEY_LEFT_PIN, 0);//input	
	gpio_pull(_gpio_fd, KEY_LEFT_PIN, 2); //pull up

	gpio_config(_gpio_fd, KEY_RIGHT_PIN, 0);//input	
	gpio_pull(_gpio_fd, KEY_RIGHT_PIN, 2); //pull up

	gpio_config(_gpio_fd, KEY_PRESS_PIN, 0);//input	
	gpio_pull(_gpio_fd, KEY_PRESS_PIN, 2); //pull up

	gpio_config(_gpio_fd, KEY1_PIN, 0);//input	
	gpio_pull(_gpio_fd, KEY1_PIN, 2); //pull up

	gpio_config(_gpio_fd, KEY2_PIN, 0);//input	
	gpio_pull(_gpio_fd, KEY2_PIN, 2); //pull up

	gpio_config(_gpio_fd, KEY3_PIN, 0);//input	
	gpio_pull(_gpio_fd, KEY3_PIN, 2); //pull up
}

int main(int argc, char** argv) {
	_gpio_fd = open("/dev/gpio", O_RDWR);
	if(_gpio_fd < 0)
		return -1;
	init_gpio();

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/joystick";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joystick");
	dev.read = joystick_read;

	device_run(&dev, mnt_point, FS_TYPE_DEV, NULL, 1);
	close(_gpio_fd);
	return 0;
}
