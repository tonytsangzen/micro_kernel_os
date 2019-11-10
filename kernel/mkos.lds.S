ENTRY(__entry)

INCLUDE linker.lds.S

SECTIONS
{
	/*kernel will be compiled with virtual address base at 2GB*/
	/*base_start_address = (0x80010000)*/
	. = base_address + start_address;

	PROVIDE(_kernel_start = .);

 .text : AT(start_address)
	{
		*(.text)
	}

	.data : 
	{ 
		*(.data) 
	}

	.bss : 
	{ 
		*(.bss COMMON)
	}
	. = ALIGN(8);
	. = . + 0x1000;
	_irq_stack = .;

	. = . + 0x10000;
	_abt_stack = .;

	. = . + 0x10000;
	_svc_stack = .;

	. = ALIGN(8);
	PROVIDE(_framebuffer_base = .); 
	. += framebuffer_size;

	. = ALIGN(4096);
	_initrd = .;
	. = . + 0x400000; /*1MB*/

	PROVIDE(_kernel_end = .);
}
