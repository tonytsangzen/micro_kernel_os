#ifndef XCNTL_H
#define XCNTL_H

enum {
	X_CNTL_NONE = 0,
	X_CNTL_NEW,
	X_CNTL_UPDATE
};

typedef struct {
	int shm_id;
	grect_t r;
} xinfo_t;

#endif
