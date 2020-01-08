MOUSED_OBJS = $(ROOT_DIR)/sbin/dev/arch/versatilepb/moused/moused.o

MOUSED = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/versatilepb_moused

PROGS += $(MOUSED)
CLEAN += $(MOUSED_OBJS)

$(MOUSED): $(MOUSED_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(MOUSED_OBJS) $(LIB_OBJS) -o $(MOUSED) $(LDFLAGS)
