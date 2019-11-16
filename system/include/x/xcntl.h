#ifndef XCNTL_H
#define XCNTL_H

enum {
	X_CNTL_NONE = 0,
	X_CNTL_NEW,
	X_CNTL_UPDATE,
	X_CNTL_GET_EVT
};

#define X_TITLE_MAX 64
typedef struct {
	int shm_id;
	grect_t r;
	char title[X_TITLE_MAX];
} xinfo_t;

#endif
