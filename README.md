# EwokOS
.Author

	Misa.Z misa.zhu@gmail.com

.About

	A microkernel OS for learning operating system. versatilepb ported well, raspi2 todo....
	-mmu
	-multi processes
	-multi thread
	-ipc
	-virtual fs service(everythig is a file)
	-very simple ramdisk for initrd
	-framebuffer device service for graphics
	-uart device service
	-SD card

.Environment & Tools

	Linux:	
		Ubuntu Linux 16.04 with "qemu-system-arm","gcc-arm-none-eabi","gdb-arm-none-eabi","fuseext2"
		installed(can install by "apt")

	Mac OSX(with brew installed):	
		brew tap PX4/homebrew-px4
		brew install gcc-arm-none-eabi-49
		brew install qemu
		(set the right PATH environment after installed)
		
	How to create/mount ext2 image in macosx
		===============prepair================
		brew install e2fsprogs
		brew cask install osxfuse
		brew install libtool 
		brew install autoconf
		brew install automake

		(download https://github.com/alperakcan/fuse-ext2)
		./autogen.sh
		CFLAGS="-idirafter/opt/gnu/include -idirafter/usr/local/include/osxfuse/ -idirafter/$(brew --prefix e2fsprogs)/include" LDFLAGS="-L/usr/local/opt/glib -L/usr/local/lib -L$(brew --prefix e2fsprogs)/lib" ./configure
		sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
		make
		sudo make install
		=================example==============
		dd if=/dev/zero of=img bs=1024 count=16384
 		mke2fs img
 		mkdir -p tmp
		fuse-ext2 -o force,rw+ img tmp
 		(copy files)
 		umount ./tmp
 		rm -r tmp
	
.make and run
	
	"cd system; make":
	  build EwokOS rootfs apps and sd file system.
	"cd kernel; make":
	  build EwokOS kernel image.
	"make run":
	  run EwokOS;
	  "qemu-system-arm -kernel build/EwokOS.bin -serial mon:stdio -initrd ../system/build/rootfs.img"
	  boot kernel file and mount initrd.
	"make debug":
	  run EwokOS at debug server-mode.
	"make gdb":
	  debug EwokOS (debug client-mode).

.commands 
	
	Most of commands are in 'rootfs/sbin' directory, like:
	ls, ps, pwd, test ......

.Source code read-guide

	kernel/arch/arm/common/boot.S
	kernel/kernel/mm/kalloc.c
	kernel/kernel/mm/mmu.c
	kernel/kernel/mm/trunkmalloc.c
	kernel/kernel/mm/kmalloc.c
	kernel/kernel/proc.c 
	kernel/kernel/sheduler.c
	kernel/kernel/syscalls.c
	kernel/kernel/mm/shm.c
	kernel/kernel/ipc.c

	rootfs/lib/src/stdlib.c
	rootfs/lib/src/unistd.c
	rootfs/lib/src/ipc.c
	rootfs/lib/src/shm.c

	rootfs/sbin/init/init.c
	rootfs/dev/initfs/initfs.c
	rootfs/dev/fbd/fbd.c
	rootfs/dev/ttyd/ttyd.c
	rootfs/bin/shell/shell.c

	Tips: Don't fall in love with assembly too much;).

. Kernel init memory map

	PhyMem        VMem         Desc
	----------------------------------------------------
	0x00000000    0xFFFF0000   interrupt table
	0x00010000    0x80010000   Kernel start (load to)
	***           ***          (_init_stack, _irq_stack, _startup_page_dir)
	***           ***          Kernel end, Kernel PageDir Table start
	+16KB         +16KB        Kernel PageDir Table end.
	+128KB        +128KB        kernel malloc base
	+32M           +32M          kernel malloc end (size=2M).
	......
	0x08000000    0x08000000   init ramdisk base (load initrd to)
	0x08020000    0x08020000   init ramdisk end (size=2M).
	......
	physical ram top           Share memory start base               
	......
	MMIO_BASE_PHY MMIO_BASE    MMIO base (arch)
	......


