INIT_OBJS = initfs/init/init.o

INIT = $(TARGET_DIR)/initfs/init

PROGS += $(INIT)
CLEAN += $(INIT_OBJS)

$(INIT): $(INIT_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(INIT_OBJS) $(LIB_OBJS) -o $(INIT) $(LDFLAGS)
	$(OBJDUMP) -D $(INIT) > $(TARGET_DIR)/asm/init.asm

