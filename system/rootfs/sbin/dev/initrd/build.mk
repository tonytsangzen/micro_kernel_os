ROOTFS_OBJS = $(ROOT_DIR)/sbin/dev/initrd/initrd.o
ROOTFS = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/initrd

PROGS += $(ROOTFS)
CLEAN += $(ROOTFS_OBJS)

$(ROOTFS): $(ROOTFS_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(ROOTFS_OBJS) $(LIB_OBJS) -o $(ROOTFS) $(LDFLAGS)
	$(OBJDUMP) -D $(ROOTFS) > $(TARGET_DIR)/asm/initrd.asm

