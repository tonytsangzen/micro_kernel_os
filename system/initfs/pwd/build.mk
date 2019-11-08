PWD_OBJS = initfs/pwd/pwd.o

PWD = $(TARGET_DIR)/initfs/pwd

PROGS += $(PWD)
CLEAN += $(PWD_OBJS)

$(PWD): $(PWD_OBJS)
	$(LD) -Ttext=100 $(PWD_OBJS) $(LIB_OBJS) -o $(PWD) $(LDFLAGS)
	$(OBJDUMP) -D $(PWD) > $(TARGET_DIR)/asm/pwd.asm

