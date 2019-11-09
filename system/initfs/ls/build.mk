LS_OBJS = initfs/ls/ls.o

LS = $(TARGET_DIR)/initfs/ls

PROGS += $(LS)
CLEAN += $(LS_OBJS)

$(LS): $(LS_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(LS_OBJS) $(LIB_OBJS) -o $(LS) $(LDFLAGS)
	$(OBJDUMP) -D $(LS) > $(TARGET_DIR)/asm/ls.asm

