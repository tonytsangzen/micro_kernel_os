#include "mm/mmu.h"
#include "mm/kmalloc.h"
#include "kstring.h"
#include "dev/fbinfo.h"
#include "dev/framebuffer.h"
#include "mailbox.h"
#include "dev/framebuffer.h"
#include "kstring.h"
#include <kernel/system.h>

static fbinfo_t _fb_info __attribute__((aligned(16)));
char* _framebuffer_base = NULL;
char* _framebuffer_end = NULL;

int32_t fb_dev_init(uint32_t w, uint32_t h, uint32_t dep) {
	property_message_tag_t tags[5];

	tags[0].proptag = FB_SET_PHYSICAL_DIMENSIONS;
	tags[0].value_buffer.fb_screen_size.width = w;
	tags[0].value_buffer.fb_screen_size.height = h;
	tags[1].proptag = FB_SET_VIRTUAL_DIMENSIONS;
	tags[1].value_buffer.fb_screen_size.width = w;
	tags[1].value_buffer.fb_screen_size.height = h;
	tags[2].proptag = FB_SET_BITS_PER_PIXEL;
	tags[2].value_buffer.fb_bits_per_pixel = dep;
	tags[3].proptag = NULL_TAG;

	// Send over the initialization
	if (send_messages(tags) != 0) {
		return -1;
	}

	_fb_info.width = tags[0].value_buffer.fb_screen_size.width;
	_fb_info.height = tags[0].value_buffer.fb_screen_size.height;
	_fb_info.vwidth = _fb_info.width;
	_fb_info.vheight = _fb_info.height;
	_fb_info.depth = 32;
	_fb_info.pitch = _fb_info.width*(_fb_info.depth/8);

	// request a framebuffer
	tags[0].proptag = FB_ALLOCATE_BUFFER;
	tags[0].value_buffer.fb_screen_size.width = 0;
	tags[0].value_buffer.fb_screen_size.height = 0;
	tags[0].value_buffer.fb_allocate_align = 16;
	tags[1].proptag = NULL_TAG;

	if (send_messages(tags) != 0) {
		return -1;
	}

	_fb_info.pointer = (uint32_t)tags[0].value_buffer.fb_allocate_res.fb_addr;
	_fb_info.size = tags[0].value_buffer.fb_allocate_res.fb_size;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;

	_framebuffer_base = (char*)_fb_info.pointer;
	_framebuffer_end = _framebuffer_base + _fb_info.height*_fb_info.width*(_fb_info.depth/8);
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

