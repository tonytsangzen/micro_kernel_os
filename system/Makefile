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
         -fno-builtin-printf -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 

LDFLAGS = -lc

LIB_DIR = lib
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
	$(LIB_DIR)/sys/wait.o \
	$(LIB_DIR)/debug.o

CLEAN = $(LIB_OBJS)

TARGET_DIR = build

all: mkdirs $(LIB_OBJS) apps 
	mkramfs/mkramfs ${TARGET_DIR}/initfs.img ${TARGET_DIR}/initfs

include mkramfs/build.mk
include initfs/initfsd/build.mk
include initfs/ttyd/build.mk
include initfs/nulld/build.mk
include initfs/init/build.mk
include initfs/shell/build.mk
include initfs/pwd/build.mk
include initfs/cat/build.mk
include initfs/ps/build.mk
include initfs/echo/build.mk
include initfs/ls/build.mk
include initfs/dump/build.mk

mkdirs:
	mkdir -p ${TARGET_DIR}/initfs
	mkdir -p ${TARGET_DIR}/asm

apps: ${PROGS}

clean:
	rm -f $(CLEAN)
	rm -fr build
