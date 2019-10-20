#include <dev/uart_basic.h>
#include <kernel/irq.h>
#include <dev/gic.h>
#include <mm/mmu.h>

/* memory mapping for the serial port */
#define UART0 ((volatile uint32_t*)(MMIO_BASE+0x001f1000))
/* serial port register offsets */
#define UART_DATA        0x00 
#define UART_FLAGS       0x18
#define UART_INT_ENABLE  0x0e
#define UART_INT_TARGET  0x0f
#define UART_INT_CLEAR   0x11

/* serial port bitmasks */
#define UART_RECEIVE  0x10
#define UART_TRANSMIT 0x20

uint8_t uart_basic_init(void) {
	put32(UART0+UART_INT_ENABLE, UART_RECEIVE);
	gic_set_irqs(IRQ_UART0);
	return 1;
}

static void uart_basic_trans(char c) {
	/* wait until transmit buffer is full */
	while (UART0[UART_FLAGS] & UART_TRANSMIT);
	/* write the character */
	put8(UART0+UART_DATA, c);
}

void uart_basic_putch(int c) {
	if(c == '\r')
		c = '\n';
	uart_basic_trans(c);
}

uint8_t uart_basic_ready_to_recv(void) {
	return get8(UART0+UART_INT_TARGET) &  UART_RECEIVE;
}

int32_t uart_basic_recv(void) {
	return get32(UART0 + UART_DATA);
}

