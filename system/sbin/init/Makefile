# tools
AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld

# flags
CFLAGS = -I. -O2 \
			   -Wstrict-prototypes \
				 -I ../../include \
				 -I ../../../kernel/include \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf -fno-builtin-strcpy -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 

LDFLAGS = -lgcc

LIB_DIR = ../../lib
LIB_OBJS=$(LIB_DIR)/svc_call.o

OBJS = ${LIB_OBJS} \
	init.o

TARGET_DIR=../../build/sbin
TARGET = ${TARGET_DIR}/init

all: $(OBJS)
	mkdir -p ${TARGET_DIR} 
	$(LD) -Ttext=100 $(OBJS) -o $(TARGET)
clean:
	rm -f $(OBJS) $(TARGET)
