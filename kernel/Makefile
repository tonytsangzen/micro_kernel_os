OS = mkos

# tools
AR = arm-none-eabi-ar
AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump

ARCH=arm/versatilepb
include arch/${ARCH}/config.mk

# flags
CFLAGS = \
	-I. -Iinclude \
	-marm $(QEMU_CFLAGS) \
	-Wstrict-prototypes \
	-pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
	-fno-builtin-printf -fno-builtin-strcpy -Wno-overlength-strings \
	-fno-builtin-exit -fno-builtin-stdio \
	-std=c99 \
	-g \
	-mcpu=$(CPU)

ASFLAGS = -g -I. -Iinclude -mcpu=$(CPU)

all: $(OS).bin 

ARCH_OBJS = \
	arch/arm/common/boot.o \
	arch/arm/common/interrupt.o \
	arch/arm/common/system.o \
	arch/arm/common/memcpy.o \
	arch/${ARCH}/arch_info.o \
	arch/${ARCH}/uart_basic.o \
	arch/${ARCH}/gic.o \
	arch/${ARCH}/timer.o 

LIB_OBJS = \
	lib/kstring.o \
	lib/svc_call.o \
	lib/vprintf.o

OBJS = ${ARCH_OBJS} \
	${LIB_OBJS} \
	kernel/mm/startup.o \
	kernel/mm/kalloc.o \
	kernel/mm/mmu.o \
	kernel/mm/trunkmalloc.o \
	kernel/mm/kmalloc.o \
	kernel/printk.o \
	kernel/init.o \
	kernel/irq.o \
	kernel/svc.o \
	kernel/proc.o \
	kernel/schedule.o \
	kernel/kernel.o

BUILD_DIR = ../build

$(OS).bin: $(OBJS) mkos.lds.S
	mkdir -p ${BUILD_DIR}
	$(LD) -L arch/${ARCH} -T mkos.lds.S $(OBJS) -o ${BUILD_DIR}/$(OS).elf
	$(OBJCOPY) -O binary ${BUILD_DIR}/$(OS).elf ${BUILD_DIR}/$(OS).bin
	$(OBJDUMP) -D ${BUILD_DIR}/$(OS).elf > ${BUILD_DIR}/$(OS).asm

run:
	qemu-system-arm $(QEMU_FLAGS) -kernel ${BUILD_DIR}/$(OS).bin

runasm:
	qemu-system-arm $(QEMU_FLAGS) -d in_asm -kernel ${BUILD_DIR}/$(OS).bin

debug:
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel ${BUILD_DIR}/$(OS).bin

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	arm-none-eabi-gdb ${BUILD_DIR}/$(OS).elf -x /tmp/gdbinit

clean:
	rm -f $(OBJS)
	rm -fr ${BUILD_DIR}
