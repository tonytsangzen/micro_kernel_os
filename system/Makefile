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
         -nostdlib \
				 -std=c99 

LDFLAGS = -lgcc

LIB_DIR = lib
LIB_OBJS = $(LIB_DIR)/svc_call.o \
	$(LIB_DIR)/unistd.o \
	$(LIB_DIR)/stdlib.o \
	$(LIB_DIR)/uart_debug.o

INIT_OBJS = ${LIB_OBJS} \
	sbin/init/init.o

TARGET_DIR = build
INIT = ${TARGET_DIR}/sbin/init

init: $(INIT_OBJS)
	mkdir -p ${TARGET_DIR}/sbin
	$(LD) -Ttext=100 $(INIT_OBJS) -o $(INIT)
	$(OBJDUMP) -D $(INIT) > $(INIT).asm

all: ${init}

clean:
	rm -f $(LIB_OBJS)
	rm -f $(INIT_OBJS) $(INIT)
	rm -fr build
