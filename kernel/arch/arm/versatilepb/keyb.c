#include <dev/keyb.h>
#include <mm/mmu.h>

#define KCNTL 0x00
#define KSTAT 0x04
#define KDATA 0x08
#define KCLK  0x0C
#define KISTA 0x10

#define KEYBOARD_BASE (MMIO_BASE+0x6000)

int32_t keyb_init() {
  put8(KEYBOARD_BASE + KCNTL, 0x10); // bit4=Enable bit0=INT on
  put8(KEYBOARD_BASE + KCLK, 8);
	return 1;
}

static int32_t _keyb_map = 0;

//0    1    2    3    4    5    6    7     8    9    A    B    C    D    E    F
const char _ltab[] = {
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0, '\t',  '`',   0,
  0,   0,   0,   0,   0,  'q', '1',  0,    0,   0,  'z', 's', 'a', 'w', '2',  0,
  0,  'c', 'x', 'd', 'e', '4', '3',  0,    0,  ' ', 'v', 'f', 't', 'r', '5',  0,
  0,  'n', 'b', 'h', 'g', 'y', '6',  0,    0,   0,  'm', 'j', 'u', '7', '8',  0,
  0,  ',', 'k', 'i', 'o', '0', '9',  0,    0,  '.', '/', 'l', ';', 'p', '-',  0,
  0,   0,  '\'',  0,  '[', '=',  0,   0,    0,   0, '\r', ']',  0, '\\',  0,   0,
  0,   0,   0,   0,   0,   0,  '\b', 0,    0,   0,   0,   0,   0,   0,   0,   0
};

const char _utab[] = {
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   '~',   0,
  0,   0,   0,   0,   0,  'Q', '!',  0,    0,   0,  'Z', 'S', 'A', 'W', '@',  0,
  0,  'C', 'X', 'D', 'E', '$', '#',  0,    0,  ' ', 'V', 'F', 'T', 'R', '%',  0,
  0,  'N', 'B', 'H', 'G', 'Y', '^',  0,    0,   0,  'M', 'J', 'U', '&', '*',  0,
  0,  '<', 'K', 'I', 'O', ')', '(',  0,    0,  '>', '?', 'L', ':', 'P', '_',  0,
  0,   0,  '"',  0,  '{', '+',  0,   0,    0,   0,  '\r','}',  0,  '|',  0,   0,
  0,   0,   0,   0,   0,   0,  '\b', 0,    0,   0,   0,   0,   0,   0,   0,   0
};

static uint8_t _held[128] = {0};

static int32_t keyb_inputch_map0(dev_t* dev, int32_t loop) {
	uint8_t scode;
	char c = 0;
	scode = get8(KEYBOARD_BASE + KDATA);

	if (scode == 0xF0) {// key release 
		return 0;
	}

	if (_held[scode] == 1 && scode) {    // next scan code following key release
		_held[scode] = 0;
		return 0;
	}
	_held[scode] = 1;

	if(scode == 0x12 || scode == 0x59 || scode == 0x14)
		return 0;

	if(_held[0x14] == 1 && _ltab[scode] == 'c') { // if control held and 'c' pressed
		//dev_kevent_push(KEV_TERMINATE);
		return 0;
	}
	else if(_held[0x14] == 1 && _ltab[scode] == '\t') {// if control held and table pressed
		//dev_kevent_push(KEV_CONSOLE_SWITCH);
		return 0;
	}
	else if(_held[0x14] == 1 && _ltab[scode] == 'd') {// if control held and 'd' pressed
		c = 0x04;
	}
	else if (_held[0x12] == 1 || _held[0x59] == 1) { // If shift key held
		c = _utab[scode];
	}
	else { // No significant keys held               
		c = _ltab[scode];
	}

	if(c == 0)
		return 0;

	charbuf_push(&dev->buffer, c, loop);
	return 0;
}

/*
keyboard map2
*/

/* Scan codes to ASCII for unshifted keys; unused keys are left out */
static char unsh[] = {
	0,033,'1','2','3','4','5','6',        '7','8','9','0', '-','=','\b','\t',
	'q','w','e','r','t','y','u','i',      'o','p','[',']', '\r', 0,'a','s',
	'd','f','g','h','j','k','l',';',      '\'',  '`',  0,  0,  'z','x','c','v',
	'b','n','m',',','.','/', 0,'*',        0, ' '
};

/* Scan codes to ASCII for shifted keys; unused keys are left out */
static char sh[] = {
	0,033,'!','@','#','$','%','^',        '&','*','(',')', '_', '+','\b','\t',
	'Q','W','E','R','T','Y','U','I',      'O','P','{','}', '\r', 0, 'A', 'S',
	'D','F','G','H','J','K','L',':',       '"', '~', 0, '|', 'Z', 'X','C', 'V',
	'B','N','M','<','>','?',0,'*',         0, ' '
};

// kbd_handler1() for scan code set 1
static int32_t keyb_inputch_map1(dev_t* dev, int32_t loop) {
	uint8_t scode, c;
	static uint32_t t2 = 0;
	scode = get8(KEYBOARD_BASE + KDATA);

	if(scode == 182 || scode == 170) { 
		t2 = 0;
		return 0;
	}
	else if(scode == 54 || scode == 42) { 
		t2 = 1;
		return 0;
	}	
	else if(scode & 0x80) {
		return 0;
	}

	c = t2==0 ? unsh[scode] : sh[scode];
	if(c == 0)
		return 0;

	charbuf_push(&dev->buffer, c, loop);
	return 0;
}

int32_t keyb_inputch(dev_t* dev, int32_t loop) {
	if(_keyb_map == 0)
		return keyb_inputch_map0(dev, loop);
	else if(_keyb_map == 1)
		return keyb_inputch_map1(dev, loop);
	return -1;
}

int32_t keyb_dev_op(dev_t* dev, int32_t opcode, int32_t arg) {
	(void)dev;

	if(opcode == DEV_OP_SET) {
		_keyb_map = arg;
	}
	return 0;
}
