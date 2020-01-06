#ifndef KEVENT_H
#define KEVENT_H

#include <kernel/kevent_type.h>
#include <rawdata.h>

typedef struct st_kevent {
	int32_t type;
	rawdata_t data;

	struct st_kevent *next;
	struct st_kevent *prev;
} kevent_t;

extern int32_t kevent_push(int32_t type, rawdata_t* data);
extern int32_t kevent_pop(int32_t *type, rawdata_t* data);
extern void    kevent_init(void);

#endif
