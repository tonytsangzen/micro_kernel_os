TEST_OBJS = initfs/test/test.o

TEST = $(TARGET_DIR)/initfs/test

PROGS += $(TEST)
CLEAN += $(TEST_OBJS)

$(TEST): $(TEST_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(TEST_OBJS) $(LIB_OBJS) -o $(TEST) $(LDFLAGS)
	$(OBJDUMP) -D $(TEST) > $(TARGET_DIR)/asm/test.asm

