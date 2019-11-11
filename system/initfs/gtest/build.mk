GTEST_OBJS = initfs/gtest/gtest.o

GTEST = $(TARGET_DIR)/initfs/gtest

PROGS += $(GTEST)
CLEAN += $(GTEST_OBJS)

$(GTEST): $(GTEST_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(GTEST_OBJS) $(LIB_OBJS) -o $(GTEST) $(LDFLAGS)
	$(OBJDUMP) -D $(GTEST) > $(TARGET_DIR)/asm/gtest.asm

