NULLD_OBJS = $(ROOT_DIR)/sbin/nulld/nulld.o

NULLD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/nulld

PROGS += $(NULLD)
CLEAN += $(NULLD_OBJS)

$(NULLD): $(NULLD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(NULLD_OBJS) $(LIB_OBJS) -o $(NULLD) $(LDFLAGS)
	$(OBJDUMP) -D $(NULLD) > $(TARGET_DIR)/asm/nulld.asm

