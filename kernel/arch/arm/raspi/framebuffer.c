#include "mm/mmu.h"
#include "mm/kmalloc.h"
#include "mm/kalloc.h"
#include "kstring.h"
#include "dev/fbinfo.h"
#include "dev/framebuffer.h"
#include "mailbox.h"
#include "dev/framebuffer.h"
#include "kstring.h"
#include <kernel/system.h>
#include <kernel/kernel.h>
#include <graph.h>

static fbinfo_t _fb_info __attribute__((aligned(16)));
char* _framebuffer_base = NULL;
char* _framebuffer_end = NULL;

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t vwidth;
	uint32_t vheight;
	uint32_t bytes;
	uint32_t depth;
	uint32_t ignorex;
	uint32_t ignorey;
	void * pointer;
	uint32_t size;
} fb_init_t;

int32_t __attribute__((optimize("O0"))) fb_dev_init(uint32_t w, uint32_t h, uint32_t dep) {
//int32_t fb_dev_init(uint32_t w, uint32_t h, uint32_t dep) {
	fb_init_t fbinit;
	mail_message_t msg;

	dep = 16;
	memset(&fbinit, 0, sizeof(fb_init_t));
	fbinit.width = w;
	fbinit.height = h;
	fbinit.vwidth = fbinit.width;
	fbinit.vheight = fbinit.height;
	fbinit.depth = dep;

	msg.data = ((uint32_t)&fbinit + 0x40000000) >> 4;//gpu address add 0x40000000 with l2 cache enabled.
	//msg.data = ((uint32_t)&fbinit + 0xC0000000) >> 4;//gpu address add 0x40000000 with l2 cache disabled.
	mailbox_send(FRAMEBUFFER_CHANNEL, &msg);
	mailbox_read(FRAMEBUFFER_CHANNEL, &msg);

	if (!msg.data) {
		return -1;
	}

	_fb_info.width = fbinit.width;
	_fb_info.height = fbinit.height;
	_fb_info.vwidth = fbinit.width;
	_fb_info.vheight = fbinit.height;
	_fb_info.depth = fbinit.depth;
	_fb_info.pitch = _fb_info.width*(_fb_info.depth/8);

	_fb_info.pointer = (uint32_t)fbinit.pointer - 0x40000000;
	_fb_info.size = fbinit.size;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;

	_framebuffer_base = (char*)_fb_info.pointer;
	if((uint32_t)_framebuffer_base < KERNEL_BASE) {
		_framebuffer_base = (char*)P2V(_framebuffer_base);
	}
	_framebuffer_end = _framebuffer_base + _fb_info.size;
	_fb_info.pointer = (uint32_t)_framebuffer_base;
	//map_pages(_kernel_vm, (uint32_t)_framebuffer_base, (uint32_t)(_framebuffer_base), (uint32_t)(_framebuffer_end), AP_RW_D);
	map_pages(_kernel_vm, (uint32_t)_framebuffer_base, V2P(_framebuffer_base), V2P(_framebuffer_end), AP_RW_D);
	kmake_hole((uint32_t)_framebuffer_base, (uint32_t)_framebuffer_end);
	return 0;
}

inline fbinfo_t* fb_get_info(void) {
	return &_fb_info;
}

int32_t fb_dev_op(dev_t* dev, int32_t opcode, int32_t arg) {
	(void)dev;
	if(opcode == DEV_OP_INFO) {
		fbinfo_t* info = (fbinfo_t*)arg;
		memcpy(info, &_fb_info, sizeof(fbinfo_t));
	}
	return 0;
}

int32_t fb_dev_write(dev_t* dev, const void* buf, uint32_t size) {
	(void)dev;
	uint32_t sz = (_fb_info.depth/8) * _fb_info.width * _fb_info.height;
	if(size > sz)
		size = sz;
	if(_fb_info.depth == 32) 
		memcpy((void*)_fb_info.pointer, buf, size);
	else if(_fb_info.depth == 16) 
		dup16((uint16_t*)_fb_info.pointer, (uint32_t*)buf, _fb_info.width, _fb_info.height);
	return (int32_t)size;
}

