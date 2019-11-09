CAT_OBJS = initfs/cat/cat.o

CAT = $(TARGET_DIR)/initfs/cat

PROGS += $(CAT)
CLEAN += $(CAT_OBJS)

$(CAT): $(CAT_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(CAT_OBJS) $(LIB_OBJS) -o $(CAT) $(LDFLAGS)
	$(OBJDUMP) -D $(CAT) > $(TARGET_DIR)/asm/cat.asm

