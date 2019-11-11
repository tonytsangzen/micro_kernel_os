#include <dev/device.h>
#include <dev/kdevice.h>
#include <dev/uart.h>
#include <dev/mouse.h>
#include <dev/keyb.h>
#include <dev/framebuffer.h>
#include <kstring.h>

static dev_t _devs[DEV_NUM];

static int32_t char_dev_read(dev_t* dev, void* data, uint32_t size) {
	if(dev == NULL)
		return -1;

	int32_t i;
	for(i=0; i<(int32_t)size; i++) {
		char c;
		if(charbuf_pop(&dev->buffer, &c) != 0)
			break;
		((char*)data)[i] = c;
	}
	return i;
}

void dev_init(void) {
	uart_init();
	keyb_init();
	mouse_init();

	dev_t* dev;

	dev = &_devs[DEV_UART0];
	memset(dev, 0, sizeof(dev_t));
	dev->type = DEV_TYPE_CHAR;
	dev->ready = uart_ready;
	dev->inputch = uart_inputch;
	dev->read = char_dev_read;
	dev->write = uart_write;

	dev = &_devs[DEV_KEYB];
	memset(dev, 0, sizeof(dev_t));
	dev->type = DEV_TYPE_CHAR;
	dev->inputch = keyb_inputch;
	dev->read = char_dev_read;

	dev = &_devs[DEV_MOUSE];
	memset(dev, 0, sizeof(dev_t));
	dev->type = DEV_TYPE_CHAR;
	dev->read = char_dev_read;

	fb_dev_init(RES_1024x768);
	dev = &_devs[DEV_FRAMEBUFFER];
	memset(dev, 0, sizeof(dev_t));
	dev->type = DEV_TYPE_MEM;
	dev->write = fb_dev_write;
	dev->op = fb_dev_op;
}

dev_t* get_dev(uint32_t type) {
	if(type >= DEV_NUM)
		return NULL;
	return &_devs[type];
}

int32_t dev_op(dev_t* dev, int32_t opcode, int32_t arg) {
	if(dev->op == NULL)
		return -1;
	return dev->op(dev, opcode, arg);
}

int32_t dev_ready(dev_t* dev) {
	if(dev->ready == NULL)
		return -1;
	return dev->ready(dev);
}

/*return : -1 for error/closed, 0 for retry*/
int32_t dev_read(dev_t* dev, void* data, uint32_t size) {
	if(dev->read == NULL)
		return -1;
	return dev->read(dev, data, size);
}

/*return : -1 for error/closed, 0 for retry*/
int32_t dev_write(dev_t* dev, void* data, uint32_t size) {
	if(dev->write == NULL)
		return -1;
	return dev->write(dev, data, size);
}

