.global __entry
__entry:
	msr cpsr, #0xD2             @ enter IRQ mode with IRQ and FIQ interrupts disabled
	ldr sp, = _irq_stack        @ initialise IRQ mode stack

	msr cpsr, #0xD7             @ enter ABT mode with IRQ and FIQ interrupts disabled
	ldr sp, = _abt_stack        @ initialise ABT mode stack

	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_stack        @ initialise SVC mode stack

	bl __enable_paging /*system.S*/
  bl __jump2_high_mem /*system.S*/

	bl _copy_interrupt_table
	bl __use_high_interrupts  /* (system.S*/
	bl __enable_interrupts /*system.S*/
	bl _kernel_entry_c
