RASPI2_MOUSED_OBJS = $(ROOT_DIR)/sbin/dev/arch/raspi2/moused/moused.o

RASPI2_MOUSED = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi2/moused

PROGS += $(RASPI2_MOUSED)
CLEAN += $(RASPI2_MOUSED_OBJS)

$(RASPI2_MOUSED): $(RASPI2_MOUSED_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI2_MOUSED_OBJS) $(LIB_OBJS) -o $(RASPI2_MOUSED) $(LDFLAGS)