#include <unistd.h>
#include <stdio.h>
#include <sys/mmio.h>


enum {
	// The GPIO registers base address.
	GPIO_BASE_OFF = 0x00200000, 

	// The offsets for reach register.

	// Controls actuation of pull up/down to ALL GPIO pins.
	GPPUD = (GPIO_BASE_OFF + 0x94),

	// Controls actuation of pull up/down for specific GPIO pin.
	GPPUDCLK0 = (GPIO_BASE_OFF + 0x98),

	// The base address for UART.
	UART0_BASE_OFF = 0x00201000, 

	// The offsets for reach register for the UART.
	UART0_DR = (UART0_BASE_OFF + 0x00),
	UART0_RSRECR = (UART0_BASE_OFF + 0x04),
	UART0_FR = (UART0_BASE_OFF + 0x18),
	UART0_ILPR = (UART0_BASE_OFF + 0x20),
	UART0_IBRD = (UART0_BASE_OFF + 0x24),
	UART0_FBRD = (UART0_BASE_OFF + 0x28),
	UART0_LCRH = (UART0_BASE_OFF + 0x2C),
	UART0_CR = (UART0_BASE_OFF + 0x30),
	UART0_IFLS = (UART0_BASE_OFF + 0x34),
	UART0_IMSC = (UART0_BASE_OFF + 0x38),
	UART0_RIS = (UART0_BASE_OFF + 0x3C),
	UART0_MIS = (UART0_BASE_OFF + 0x40),
	UART0_ICR = (UART0_BASE_OFF + 0x44),
	UART0_DMACR = (UART0_BASE_OFF + 0x48),
	UART0_ITCR = (UART0_BASE_OFF + 0x80),
	UART0_ITIP = (UART0_BASE_OFF + 0x84),
	UART0_ITOP = (UART0_BASE_OFF + 0x88),
	UART0_TDR = (UART0_BASE_OFF + 0x8C),
};

static uint32_t _mmio_base = 0;

static void uart_trans(char c) {
	// Wait for UART to become ready to transmit.
	while (get32(_mmio_base+UART0_FR) & (1 << 5)) {}
	if(c == '\r')
		c = '\n';
	put32(_mmio_base+UART0_DR, c);
}
/*
static int32_t uart_ready_to_recv(void) {
	if(get32(_mmio_base+UART0_FR) & (1 << 4)) 
		return -1;
	return 0;
}

static int32_t uart_recv(void) {
	return get32(_mmio_base+UART0_DR);
}
*/

static int32_t uart_write(const void* data, uint32_t size) {
  int32_t i;
  for(i=0; i<(int32_t)size; i++) {
    char c = ((char*)data)[i];
    uart_trans(c);
  }
  return i;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	_mmio_base = mmio_map();
	if(_mmio_base == 0)
		return -1;
	uart_write("test\n", 5);
  return 0;
}

