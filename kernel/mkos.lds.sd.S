ENTRY(__entry)

INCLUDE linker.lds.S

SECTIONS
{
	/*kernel will be compiled with virtual address base at 2GB*/
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
		*(.bss*)
	}
	. = ALIGN(8);
	. = . + 0x10000;
	_irq_stack = .;

	. = . + 0x10000;
	_abt_stack = .;

	. = . + 0x10000;
	_svc_stack = .;

	. = ALIGN(8);
	PROVIDE(_framebuffer_base_raw = .); 
	. += framebuffer_size;
	PROVIDE(_framebuffer_end_raw = .); 

	. = ALIGN(8);
	PROVIDE(_kernel_end = .);
}
