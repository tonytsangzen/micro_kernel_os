INITFSD_OBJS = $(ROOT_DIR)/sbin/initfsd/initfsd.o
INITFSD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/initfsd

PROGS += $(INITFSD)
CLEAN += $(INITFSD_OBJS)

$(INITFSD): $(INITFSD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(INITFSD_OBJS) $(LIB_OBJS) -o $(INITFSD) $(LDFLAGS)
	$(OBJDUMP) -D $(INITFSD) > $(TARGET_DIR)/asm/initfsd.asm

