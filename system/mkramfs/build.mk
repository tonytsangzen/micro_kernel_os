MKRAMFS = mkramfs/mkramfs

PROGS += $(MKRAMFS)
CLEAN += $(MKRAMFS)

$(MKRAMFS): mkramfs/mkramfs.c
	gcc -O2 mkramfs/mkramfs.c -o $(MKRAMFS)
