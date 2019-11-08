#include<dev/devicetype.h>
#include <dev/device.h>
#include <dev/uart_basic.h>
#include <kstring.h>

static charbuf_t _devbufs[DEV_NUM];

void dev_init(void) {
	int i;
	for(i=0; i<DEV_NUM; i++) {
		memset(&_devbufs[i], 0, sizeof(charbuf_t));
	}

	uart_basic_init();
}

charbuf_t* dev_getbuf(uint32_t type) {
	if(type >= DEV_NUM)
		return NULL;
	return &_devbufs[type];
}

int32_t dev_read(uint32_t type, void* data, uint32_t size) {
	charbuf_t* buf = dev_getbuf(type);
	if(buf == NULL)
		return -1;
	
	int32_t i;
	for(i=0; i<(int32_t)size; i++) {
		char c;
		if(charbuf_pop(buf, &c) != 0)
			break;
		((char*)data)[i] = c;
	}
	return i;
}

int32_t dev_write(uint32_t type, void* data, uint32_t size) {
	int32_t i;
	for(i=0; i<(int32_t)size; i++) {
		char c = ((char*)data)[i];
		if(type == DEV_UART0)
			uart_basic_putch(c);
	}
	return i;
}

