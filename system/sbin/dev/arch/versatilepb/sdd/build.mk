VERSATILEPB_SDD_OBJS = $(ROOT_DIR)/sbin/dev/arch/versatilepb/sdd/sdd.o
VERSATILEPB_SDD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/versatilepb/sdd

PROGS += $(VERSATILEPB_SDD)
CLEAN += $(VERSATILEPB_SDD_OBJS)

$(VERSATILEPB_SDD): $(VERSATILEPB_SDD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(VERSATILEPB_SDD_OBJS) $(LIB_OBJS) -o $(VERSATILEPB_SDD) $(LDFLAGS)
