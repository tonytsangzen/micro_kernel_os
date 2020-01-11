RASPI_KEYBD_OBJS = $(ROOT_DIR)/sbin/dev/arch/raspi/keybd/keybd.o

RASPI_KEYBD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/raspi/keybd

PROGS += $(RASPI_KEYBD)
CLEAN += $(RASPI_KEYBD_OBJS)

$(RASPI_KEYBD): $(RASPI_KEYBD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(RASPI_KEYBD_OBJS) $(LIB_OBJS) -o $(RASPI_KEYBD) $(LDFLAGS)