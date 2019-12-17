#include "mm/mmu.h"
#include "kstring.h"
#include "dev/fbinfo.h"
#include "dev/framebuffer.h"
#include "mailbox.h"
#include "dev/framebuffer.h"
#include "kstring.h"
#include <kernel/system.h>

#define VIDEO_FB_CHANNEL MAIL_CH_FBUFF
#define VIDEO_INIT_RETRIES 3
#define VIDEO_INITIALIZED 0
#define VIDEO_ERROR_RETURN 1
#define VIDEO_ERROR_POINTER 2

#define VIDEO_FB_CHANNEL MAIL_CH_FBUFF
#define VIDEO_INIT_RETRIES 3
#define VIDEO_INITIALIZED 0
#define VIDEO_ERROR_RETURN 1
#define VIDEO_ERROR_POINTER 2

#define MAIL_CH_FBUFF 0x00000001

int32_t video_init(fbinfo_t *p_fbinfo) {
	uint32_t init = VIDEO_INIT_RETRIES;
	uint32_t test, addr = ((uint32_t)p_fbinfo);
	while(init>0) {
		__mem_barrier();
		mailbox_write(VIDEO_FB_CHANNEL,addr);
		__mem_barrier();
		test = mailbox_read(VIDEO_FB_CHANNEL);
		__mem_barrier();
		if (test) 
			test = VIDEO_ERROR_RETURN;
		else if(p_fbinfo->pointer == 0x0)
			test = VIDEO_ERROR_POINTER;
		else { 
			test = VIDEO_INITIALIZED; break; 
		}
		init--;
	}
	return test;
}

uint32_t fb_dev_get_size(void) {
	return 1*MB;
}

fbinfo_t _fb_info __attribute__((aligned(16)));

char* _framebuffer_base = NULL;
char* _framebuffer_end = NULL;

int32_t fb_dev_init(int32_t res) {
	(void)res;
	tags_info_t info;
	mailbox_get_video_info(&info);
	/** initialize fbinfo */
	_fb_info.height = info.fb_height;
	_fb_info.width = info.fb_width;
	_fb_info.vheight = info.fb_height;
	_fb_info.vwidth = info.fb_width;
	_fb_info.pitch = 0;
	_fb_info.depth = 32;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;
	_fb_info.pointer = 0;
	_fb_info.size = 0;

	int32_t r = video_init(&_fb_info);
	if(r != 0)
		return -1;
	_framebuffer_base = (char*)_fb_info.pointer;
	_framebuffer_end = _framebuffer_base + _fb_info.height*_fb_info.width*4;
	return 0;
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
	memcpy((void*)_fb_info.pointer, buf, size);
	return (int32_t)size;
}

