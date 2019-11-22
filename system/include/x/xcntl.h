#ifndef XCNTL_H
#define XCNTL_H

enum {
	X_CNTL_NONE = 0,
	X_CNTL_NEW,
	X_CNTL_UPDATE,
	X_CNTL_SCR_INFO,
	X_CNTL_GET_EVT
};

#define X_STYLE_NORMAL    0x0
#define X_STYLE_NO_FRAME  0x1
#define X_STYLE_NO_TITLE  0x2

#define X_TITLE_MAX 64
typedef struct {
	int shm_id;
	int style;
	grect_t r;
	char title[X_TITLE_MAX];
} xinfo_t;

typedef struct {
	int id;
	gsize_t size;
} xscreen_t;

#endif
