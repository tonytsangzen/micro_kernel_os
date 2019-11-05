#ifndef VDEVICE_H
#define VDEVICE_H

#include <fsinfo.h>

typedef struct {
	int (*ctrl)(fsinfo_t* node, int cmd, int arg0, int arg1, int arg2);
} vdevice_t;

#endif
