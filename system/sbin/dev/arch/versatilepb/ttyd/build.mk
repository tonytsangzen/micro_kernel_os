VERSATILEPB_TTYD_OBJS = $(ROOT_DIR)/sbin/dev/arch/versatilepb/ttyd/ttyd.o

VERSATILEPB_TTYD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/versatilepb/ttyd

PROGS += $(VERSATILEPB_TTYD)
CLEAN += $(VERSATILEPB_TTYD_OBJS)

$(VERSATILEPB_TTYD): $(VERSATILEPB_TTYD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(VERSATILEPB_TTYD_OBJS) $(LIB_OBJS) -o $(VERSATILEPB_TTYD) $(LDFLAGS)
