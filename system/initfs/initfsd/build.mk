INITFSD_OBJS = initfs/initfsd/initfsd.o

INITFSD = $(TARGET_DIR)/initfs/initfsd

PROGS += $(INITFSD)
CLEAN += $(INITFSD_OBJS)

$(INITFSD): $(INITFSD_OBJS)
	$(LD) -Ttext=100 $(INITFSD_OBJS) $(LIB_OBJS) -o $(INITFSD) $(LDFLAGS)
	$(OBJDUMP) -D $(INITFSD) > $(TARGET_DIR)/asm/initfsd.asm

