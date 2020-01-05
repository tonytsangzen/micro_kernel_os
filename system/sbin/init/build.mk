INIT_OBJS = $(ROOT_DIR)/sbin/init/init.o

INIT = $(TARGET_DIR)/$(ROOT_DIR)/sbin/init

PROGS += $(INIT)
CLEAN += $(INIT_OBJS)

$(INIT): $(INIT_OBJS) $(LIB_OBJS) $(LIB_CONSOLE_OBJS)
	$(LD) -Ttext=100 $(INIT_OBJS) $(LIB_OBJS) $(LIB_CONSOLE_OBJS) -o $(INIT) $(LDFLAGS)
	$(OBJDUMP) -D $(INIT) > $(BUILD_DIR)/asm/init.asm
