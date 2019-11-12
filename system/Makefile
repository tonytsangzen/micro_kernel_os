# tools
AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

# flags
CFLAGS = -Wstrict-prototypes \
				 -I. -g \
				 -I include \
				 -I ../kernel/include \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf \
         -fno-builtin-snprintf \
         -fno-builtin-sprintf \
				 -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 

LDFLAGS = -lc

LIB_DIR = lib
GRAPH_OBJS =	$(LIB_DIR)/graph/graph.o \
	$(LIB_DIR)/graph/font.o \
	$(LIB_DIR)/graph/font4x6.o \
	$(LIB_DIR)/graph/font5x12.o \
	$(LIB_DIR)/graph/font6x8.o \
	$(LIB_DIR)/graph/font7x9.o \
	$(LIB_DIR)/graph/font8x8.o \
	$(LIB_DIR)/graph/font9x8.o \
	$(LIB_DIR)/graph/font8x10.o \
	$(LIB_DIR)/graph/font8x16.o \
	$(LIB_DIR)/graph/font9x16.o \
	$(LIB_DIR)/graph/font12x16.o \
	$(LIB_DIR)/graph/font10x20.o \
	$(LIB_DIR)/graph/font12x24.o \
	$(LIB_DIR)/graph/font16x32.o

LIB_OBJS = $(LIB_DIR)/svc_call.o \
	$(LIB_DIR)/cmain.o \
	$(LIB_DIR)/trunk.o \
	$(LIB_DIR)/tstr.o \
	$(LIB_DIR)/vfs.o \
	$(LIB_DIR)/ramfs.o \
	$(LIB_DIR)/vprintf.o \
	$(LIB_DIR)/unistd.o \
	$(LIB_DIR)/stdlib.o \
	$(LIB_DIR)/ipc.o \
	$(LIB_DIR)/vdevice.o \
	$(LIB_DIR)/fcntl.o \
	$(LIB_DIR)/proto.o \
	$(LIB_DIR)/stdio.o \
	$(LIB_DIR)/basic_math.o \
	$(LIB_DIR)/console.o \
	$(LIB_DIR)/shm.o \
	$(LIB_DIR)/sys/wait.o \
	$(GRAPH_OBJS)

CLEAN = $(LIB_OBJS)

TARGET_DIR = build
ROOT_DIR = initfs

all: mkdirs $(LIB_OBJS) apps 
	mkramfs/mkramfs ${TARGET_DIR}/$(ROOT_DIR).img ${TARGET_DIR}/$(ROOT_DIR)

include mkramfs/build.mk
include $(ROOT_DIR)/sbin/init/build.mk
include $(ROOT_DIR)/sbin/initfsd/build.mk
include $(ROOT_DIR)/sbin/ttyd/build.mk
include $(ROOT_DIR)/sbin/fbd/build.mk
include $(ROOT_DIR)/sbin/keybd/build.mk
include $(ROOT_DIR)/sbin/moused/build.mk
include $(ROOT_DIR)/sbin/nulld/build.mk
include $(ROOT_DIR)/bin/shell/build.mk
include $(ROOT_DIR)/bin/pwd/build.mk
include $(ROOT_DIR)/bin/cat/build.mk
include $(ROOT_DIR)/bin/ps/build.mk
include $(ROOT_DIR)/bin/echo/build.mk
include $(ROOT_DIR)/bin/ls/build.mk
include $(ROOT_DIR)/bin/dump/build.mk
include $(ROOT_DIR)/bin/test/build.mk
include $(ROOT_DIR)/bin/gtest/build.mk
include $(ROOT_DIR)/bin/gconsole/build.mk

mkdirs:
	mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin
	mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/bin
	mkdir -p ${TARGET_DIR}/asm

apps: ${PROGS}

clean:
	rm -f $(CLEAN)
	rm -fr build
