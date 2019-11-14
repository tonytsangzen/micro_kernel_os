OS = mkos

# tools
AR = arm-none-eabi-ar
AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump

ARCH_DIR=arch
ARCH=arm/versatilepb
include ${ARCH_DIR}/${ARCH}/config.mk

# flags
CFLAGS = \
	-I. -I../include -Iinclude \
	-marm $(QEMU_CFLAGS) \
	-Wstrict-prototypes \
	-pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
	-fno-builtin-printf -fno-builtin-strcpy -Wno-overlength-strings \
	-fno-builtin-exit -fno-builtin-stdio \
	-std=c99 \
	-g \
	-mcpu=$(CPU)
	
CFLAGS += -D CONFIG_KEYMAP2

ASFLAGS = -g -I. -Iinclude -mcpu=$(CPU)

all: $(OS).bin 

ARCH_OBJS = \
	${ARCH_DIR}/arm/common/boot.o \
	${ARCH_DIR}/arm/common/interrupt.o \
	${ARCH_DIR}/arm/common/system.o \
	${ARCH_DIR}/arm/common/memcpy.o \
	${ARCH_DIR}/${ARCH}/hw_info.o \
	${ARCH_DIR}/${ARCH}/uart.o \
	${ARCH_DIR}/${ARCH}/keyb.o \
	${ARCH_DIR}/${ARCH}/mouse.o \
	${ARCH_DIR}/${ARCH}/gic.o \
	${ARCH_DIR}/${ARCH}/framebuffer.o \
	${ARCH_DIR}/${ARCH}/timer.o 

LIB_DIR = lib
LIB_OBJS = \
	${LIB_DIR}/kstring.o \
	${LIB_DIR}/buffer.o \
	${LIB_DIR}/vprintf.o \
	${LIB_DIR}/trunk.o \
	${LIB_DIR}/tstr.o \
	${LIB_DIR}/ramfs.o \
	${LIB_DIR}/charbuf.o \
	${LIB_DIR}/basic_math.o \
	${LIB_DIR}/kprintf.o

KERNEL_DIR = kernel
OBJS = ${ARCH_OBJS} \
	${LIB_OBJS} \
	${KERNEL_DIR}/mm/startup.o \
	${KERNEL_DIR}/mm/kalloc.o \
	${KERNEL_DIR}/mm/mmu.o \
	${KERNEL_DIR}/mm/trunkmalloc.o \
	${KERNEL_DIR}/mm/kmalloc.o \
	${KERNEL_DIR}/mm/shm.o \
	${KERNEL_DIR}/init.o \
	${KERNEL_DIR}/irq.o \
	${KERNEL_DIR}/svc.o \
	${KERNEL_DIR}/proc.o \
	${KERNEL_DIR}/schedule.o \
	${KERNEL_DIR}/vfs.o \
	${KERNEL_DIR}/dev/kdevice.o \
	${KERNEL_DIR}/kernel.o

BUILD_DIR = build
SYS_DIR = ../system

$(OS).bin: $(OBJS) mkos.lds.S
	mkdir -p ${BUILD_DIR}
	$(LD) -L ${ARCH_DIR}/${ARCH} -T mkos.lds.S $(OBJS) -o ${BUILD_DIR}/$(OS).elf
	$(OBJCOPY) -O binary ${BUILD_DIR}/$(OS).elf ${BUILD_DIR}/$(OS).bin
	$(OBJDUMP) -D ${BUILD_DIR}/$(OS).elf > ${BUILD_DIR}/$(OS).asm

INITFS = ${SYS_DIR}/build/initfs.img

run: ${INITFS}
	qemu-system-arm $(QEMU_FLAGS) -kernel ${BUILD_DIR}/$(OS).bin -initrd ${INITFS}

runnog: ${INITFS}
	qemu-system-arm $(QEMU_FLAGS) -kernel ${BUILD_DIR}/$(OS).bin -initrd ${INITFS} -nographic

debug: ${INITFS}
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel ${BUILD_DIR}/$(OS).bin -initrd  ${INITFS}

debugnog: ${INITFS}
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel ${BUILD_DIR}/$(OS).bin -initrd  ${INITFS} -nographic

debugasm: ${INITFS}
	qemu-system-arm $(QEMU_FLAGS) -d in_asm -gdb tcp::26000 -S -kernel ${BUILD_DIR}/$(OS).bin -initrd ${INITFS}

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	arm-none-eabi-gdb ${BUILD_DIR}/$(OS).elf -x /tmp/gdbinit

clean:
	rm -f $(OBJS)
	rm -fr ${BUILD_DIR}
