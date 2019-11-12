TTYD_OBJS = $(ROOT_DIR)/sbin/ttyd/ttyd.o

TTYD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/ttyd

PROGS += $(TTYD)
CLEAN += $(TTYD_OBJS)

$(TTYD): $(TTYD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(TTYD_OBJS) $(LIB_OBJS) -o $(TTYD) $(LDFLAGS)
	$(OBJDUMP) -D $(TTYD) > $(TARGET_DIR)/asm/ttyd.asm

