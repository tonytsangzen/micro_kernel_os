PS_OBJS = initfs/ps/ps.o

PS = $(TARGET_DIR)/initfs/ps

PROGS += $(PS)
CLEAN += $(PS_OBJS)

$(PS): $(PS_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(PS_OBJS) $(LIB_OBJS) -o $(PS) $(LDFLAGS)
	$(OBJDUMP) -D $(PS) > $(TARGET_DIR)/asm/ps.asm

