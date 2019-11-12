FBD_OBJS = $(ROOT_DIR)/sbin/fbd/fbd.o

FBD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/fbd

PROGS += $(FBD)
CLEAN += $(FBD_OBJS)

$(FBD): $(FBD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(FBD_OBJS) $(LIB_OBJS) -o $(FBD) $(LDFLAGS)
	$(OBJDUMP) -D $(FBD) > $(TARGET_DIR)/asm/fbd.asm

