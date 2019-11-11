#include "mm/mmu.h"
#include "string.h"
#include "dev/fbinfo.h"
#include "dev/framebuffer.h"

static int32_t _res = RES_640x480;

static int32_t video_init(fbinfo_t *fbinfo) {
	if(_res == RES_640x480) {
		put32((MMIO_BASE | 0x1c), 0x2c77);
		put32((MMIO_BASE | 0x00120000), 0x3f1f3f9c);
		put32((MMIO_BASE | 0x00120004), 0x090b61df); 
		put32((MMIO_BASE | 0x00120008), 0x067f1800); 
	}
	else if(_res == RES_800x600) {
		put32((MMIO_BASE | 0x1c), 0x2cac);
		put32((MMIO_BASE | 0x00120000), 0x1313a4c4);
		put32((MMIO_BASE | 0x00120004), 0x0505f6f7);
		put32((MMIO_BASE | 0x00120008), 0x071f1800); 
	}
	else {
		//1024x768
		put32((MMIO_BASE | 0x00120000), 0x3F << 2);
		put32((MMIO_BASE | 0x00120004), 767);
	}	
	
	put32((MMIO_BASE | 0x00120010), fbinfo->pointer);
	put32((MMIO_BASE | 0x00120018), 0x082b);
	return 0;
}

static fbinfo_t _fbinfo __attribute__((aligned(16)));

void fb_dev_init(int32_t res) {
	_res = res;

	if(_res == RES_640x480) {
		_fbinfo.height = 480;
		_fbinfo.width = 640;
		_fbinfo.vheight = 480;
		_fbinfo.vwidth = 640;
	}
	else if(_res == RES_800x600) {
		_fbinfo.height = 600;
		_fbinfo.width = 800;
		_fbinfo.vheight = 600;
		_fbinfo.vwidth = 800;
	}
	else {
		_fbinfo.height = 768;
		_fbinfo.width = 1024;
		_fbinfo.vheight = 768;
		_fbinfo.vwidth = 1024;
	}

	_fbinfo.pitch = 0;
	_fbinfo.depth = 32;
	_fbinfo.xoffset = 0;
	_fbinfo.yoffset = 0;
	_fbinfo.pointer = V2P(_framebuffer_base);
	_fbinfo.size = 0;

	video_init(&_fbinfo);
}

uint32_t fb_dev_get_size(void) {
	return 1*MB;
}

int32_t fb_dev_op(dev_t* dev, int32_t opcode, int32_t arg) {
	(void)dev;

	if(opcode == DEV_OP_INFO) {
		fbinfo_t* info = (fbinfo_t*)arg;
		memcpy(info, &_fbinfo, sizeof(fbinfo_t));
	}
	return 0;
}

int32_t fb_dev_write(dev_t* dev, const void* buf, uint32_t size) {
	(void)dev;
	uint32_t sz = (_fbinfo.depth/8) * _fbinfo.width * _fbinfo.height;
	if(size > sz)
		size = sz;
	memcpy((void*)_framebuffer_base, buf, size);
	return (int32_t)size;
}
