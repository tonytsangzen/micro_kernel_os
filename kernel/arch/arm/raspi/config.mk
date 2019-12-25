#CPU = arm1176jz-s
#QEMU_FLAGS = -M raspi2 -m 512M -cpu arm1176jz-s

CPU = cortex-a7
#QEMU_FLAGS = -M raspi2 -m 512M  -nographic -display none -serial mon:stdio
QEMU_FLAGS = -M raspi2 -m 512M -serial mon:stdio
ARCH_QEMU_CFLAGS = -DCPU_NUM=4 -DA_CORE
