GCONSOLE_OBJS = initfs/gconsole/gconsole.o

GCONSOLE = $(TARGET_DIR)/initfs/gconsole

PROGS += $(GCONSOLE)
CLEAN += $(GCONSOLE_OBJS)

$(GCONSOLE): $(GCONSOLE_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(GCONSOLE_OBJS) $(LIB_OBJS) -o $(GCONSOLE) $(LDFLAGS)
	$(OBJDUMP) -D $(GCONSOLE) > $(TARGET_DIR)/asm/gconsole.asm

