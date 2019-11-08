#include <dev/kdevicetype.h>
#include <dev/kdevice.h>
#include <dev/uart_basic.h>
#include <kstring.h>

static char_dev_t _char_devs[DEV_NUM];

void char_dev_init(void) {
	char_dev_t* dev;

	uart_basic_init();

	dev = &_char_devs[DEV_UART0];
	memset(dev, 0, sizeof(char_dev_t));
	dev->ready = uart_basic_ready_to_recv;
	dev->read = uart_basic_recv;
	dev->write = uart_basic_putch;
}

char_dev_t* get_char_dev(uint32_t type) {
	if(type >= DEV_NUM)
		return NULL;
	return &_char_devs[type];
}

int32_t char_dev_read(char_dev_t* dev, void* data, uint32_t size) {
	int32_t i;
	for(i=0; i<(int32_t)size; i++) {
		char c;
		if(charbuf_pop(&dev->buffer, &c) != 0)
			break;
		((char*)data)[i] = c;
	}
	return i;
}

int32_t char_dev_write(char_dev_t* dev, void* data, uint32_t size) {
	int32_t i;
	for(i=0; i<(int32_t)size; i++) {
		char c = ((char*)data)[i];
		dev->write(c);
	}
	return i;
}

