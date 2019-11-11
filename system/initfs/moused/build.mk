MOUSED_OBJS = initfs/moused/moused.o

MOUSED = $(TARGET_DIR)/initfs/moused

PROGS += $(MOUSED)
CLEAN += $(MOUSED_OBJS)

$(MOUSED): $(MOUSED_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(MOUSED_OBJS) $(LIB_OBJS) -o $(MOUSED) $(LDFLAGS)
	$(OBJDUMP) -D $(MOUSED) > $(TARGET_DIR)/asm/moused.asm

