.global interrupt_table_start, interrupt_table_end
interrupt_table_start: /*interrupt table, with syscall and irq items*/
	ldr pc, reset_entry_address /*item 0: for reset*/
	b . /*item 1: reserved for undef instruct*/
	ldr pc, syscall_entry_address /*item 2: syscall*/
	ldr pc, prefetch_abort_entry_address /*item 3: prefetchAbort*/
	ldr pc, data_abort_entry_address /*item 4: dataAbort*/
	b . /*item 5: reserved*/
	ldr pc, irq_entry_address /*item 6: irq*/

	reset_entry_address: .word __entry
	syscall_entry_address: .word syscall_entry
	irq_entry_address: .word irq_entry
	prefetch_abort_entry_address: .word prefetch_abort_entry
	data_abort_entry_address: .word prefetch_abort_entry
interrupt_table_end:

syscall_entry:

irq_entry:
	sub   lr, lr, #4              @ correct return address
	msr   cpsr, #0xD2             @ enter IRQ mode with IRQ and FIQ interrupts disabled
	sub   sp, sp, #60             @ update SVC mode stack
	stmia sp, { r0-r12, sp, lr }^ @ store  USR registers
	mrs   r0, spsr                @ get    USR        CPSR
	stmdb sp!, { r0, lr }         @ store  USR PC and CPSR
	mov   r0, sp

	bl    irq_handler             @ invoke high-level C function

	ldmia sp!, { r0, lr }         @ load   USR mode PC and CPSR
	msr   spsr, r0                @ set    USR mode        CPSR
	ldmia sp, { r0-r12, sp, lr }^ @ load   USR mode registers
	add   sp, sp, #60             @ update SVC mode SP
	movs  pc, lr                  @ return from interrupt

prefetch_abort_entry:

data_abort_entry:

