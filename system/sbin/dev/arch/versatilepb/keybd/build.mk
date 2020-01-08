KEYBD_OBJS = $(ROOT_DIR)/sbin/dev/arch/versatilepb/keybd/keybd.o

KEYBD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/versatilepb_keybd

PROGS += $(KEYBD)
CLEAN += $(KEYBD_OBJS)

$(KEYBD): $(KEYBD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(KEYBD_OBJS) $(LIB_OBJS) -o $(KEYBD) $(LDFLAGS)
