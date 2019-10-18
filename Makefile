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
	arch/${ARCH}/uart_basic.o 

LIB_OBJS = \
	lib/kstring.o \
	lib/vprintf.o

OBJS = ${ARCH_OBJS} \
	${LIB_OBJS} \
	kernel/mm/startup.o \
	kernel/mm/kalloc.o \
	kernel/mm/mmu.o \
	kernel/printk.o \
	kernel/init.o \
	kernel/kernel.o

$(OS).bin: $(OBJS) mkos.lds.S
	mkdir -p build
	$(LD) -L arch/${ARCH} -T mkos.lds.S $(OBJS) -o build/$(OS).elf
	$(OBJCOPY) -O binary build/$(OS).elf build/$(OS).bin
	$(OBJDUMP) -D build/$(OS).elf > build/$(OS).asm

run:
	qemu-system-arm $(QEMU_FLAGS) -kernel build/$(OS).bin

runasm:
	qemu-system-arm $(QEMU_FLAGS) -d in_asm -kernel build/$(OS).bin

debug:
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel build/$(OS).bin

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	arm-none-eabi-gdb build/$(OS).elf -x /tmp/gdbinit

clean:
	rm -f $(OBJS)
	rm -fr build
