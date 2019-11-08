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
	$(LIB_DIR)/vfs.o \
	$(LIB_DIR)/ramfs.o \
	$(LIB_DIR)/vprintf.o \
	$(LIB_DIR)/unistd.o \
	$(LIB_DIR)/stdlib.o \
	$(LIB_DIR)/ipc.o \
	$(LIB_DIR)/vdevice.o \
	$(LIB_DIR)/fcntl.o \
	$(LIB_DIR)/proto.o \
	$(LIB_DIR)/sys/wait.o \
	$(LIB_DIR)/debug.o

TARGET_DIR = build

INIT_OBJS = ${LIB_OBJS} \
	initfs/init/init.o
INIT = ${TARGET_DIR}/initfs/init

INITFSD_OBJS = ${LIB_OBJS} \
	initfs/initfsd/initfsd.o
INITFSD = ${TARGET_DIR}/initfs/initfsd

SHELL_OBJS = ${LIB_OBJS} \
	initfs/shell/shell.o
SHELL_ = ${TARGET_DIR}/initfs/shell

TTYD_OBJS = ${LIB_OBJS} \
	initfs/ttyd/ttyd.o
TTYD = ${TARGET_DIR}/initfs/ttyd

all: init initfsd shell ttyd
	mkramfs/mkramfs ${TARGET_DIR}/initfs.img ${TARGET_DIR}/initfs

init: $(INIT_OBJS)
	mkdir -p ${TARGET_DIR}/initfs
	mkdir -p ${TARGET_DIR}/asm
	$(LD) -Ttext=100 $(INIT_OBJS) -o $(INIT) ${LDFLAGS}
	$(OBJDUMP) -D ${INIT} > ${TARGET_DIR}/asm/init.asm

initfsd: $(INITFSD_OBJS)
	$(LD) -Ttext=100 $(INITFSD_OBJS) -o $(INITFSD) ${LDFLAGS}
	$(OBJDUMP) -D ${INITFSD} > ${TARGET_DIR}/asm/initfsd.asm

shell: $(SHELL_OBJS)
	$(LD) -Ttext=100 $(SHELL_OBJS) -o $(SHELL_) ${LDFLAGS}
	$(OBJDUMP) -D ${SHELL_} > ${TARGET_DIR}/asm/shell.asm

ttyd: $(TTYD_OBJS)
	$(LD) -Ttext=100 $(TTYD_OBJS) -o $(TTYD) ${LDFLAGS}
	$(OBJDUMP) -D ${TTYD} > ${TARGET_DIR}/asm/ttyd.asm

clean:
	rm -f $(LIB_OBJS)
	rm -f $(INIT_OBJS)
	rm -f $(INITFSD_OBJS)
	rm -f $(SHELL_OBJS)
	rm -f $(TTYD_OBJS)
	rm -fr build
