SHELL__OBJS = initfs/shell/shell.o

SHELL_ = $(TARGET_DIR)/initfs/shell

PROGS += $(SHELL_)
CLEAN += $(SHELL__OBJS)

$(SHELL_): $(SHELL__OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(SHELL__OBJS) $(LIB_OBJS) -o $(SHELL_) $(LDFLAGS)
	$(OBJDUMP) -D $(SHELL_) > $(TARGET_DIR)/asm/shell.asm

