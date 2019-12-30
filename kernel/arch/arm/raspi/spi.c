/*----------------------------------------------------------------------------*/
/**
 * - for accessing spi
 *   = this is using spi0 (spi-specific interface)
 *   = bcm2835 has 2 more mini-spi interfaces (spi1/spi2)
 *     $ part of auxiliary peripheral (along with mini-uart)
 *     $ NOT using these
**/
#include <mm/mmu.h>
#include <dev/gpio.h>
#include <dev/spi.h>

#define SPI0_OFFSET 0x00204000
#define SPI1_OFFSET 0x00215080
#define SPI2_OFFSET 0x002150C0

#define SPI_BASE (_mmio_base | SPI0_OFFSET)
#define SPI_ENABLES    (_mmio_base | 0x00215004)

#define SPI_CS_REG   (SPI_BASE+0x00)
#define SPI_FIFO_REG (SPI_BASE+0x04)
#define SPI_CLK_REG  (SPI_BASE+0x08)
#define SPI_DLEN_REG (SPI_BASE+0x0C)
#define SPI_LTOH_REG (SPI_BASE+0x10)
#define SPI_DC_REG   (SPI_BASE+0x14)

/* control status bits */
#define SPI_CNTL_CSPOL2 0x00800000
#define SPI_CNTL_CSPOL1 0x00400000
#define SPI_CNTL_CSPOL0 0x00200000
#define SPI_STAT_RXFULL 0x00100000
#define SPI_STAT_RXREAD 0x00080000
#define SPI_STAT_TXDATA 0x00040000
#define SPI_STAT_RXDATA 0x00020000
#define SPI_STAT_TXDONE 0x00010000
#define SPI_CNTL_READEN 0x00001000
#define SPI_CNTL_ADCS   0x00000800
#define SPI_CNTL_INTRXR 0x00000400
#define SPI_CNTL_INTRDN 0x00000200
#define SPI_CNTL_DMA_EN 0x00000100
#define SPI_CNTL_TRXACT 0x00000080
#define SPI_CNTL_CSPOL  0x00000040
#define SPI_CNTL_CLMASK 0x00000030
#define SPI_CNTL_CLR_RX 0x00000020
#define SPI_CNTL_CLR_TX 0x00000010
#define SPI_CNTL_CPOL   0x00000008
#define SPI_CNTL_CPHA   0x00000004
#define SPI_CNTL_CSMASK 0x00000003
#define SPI_CNTL_CS0    0x00000000
#define SPI_CNTL_CS1    0x00000001
#define SPI_CNTL_CS2    0x00000002

#define SPI0_SCLK 11
#define SPI0_MOSI 10
#define SPI0_MISO 9
#define SPI0_CE0N 8
#define SPI0_CE1N 7
#define SPI_SCLK SPI0_SCLK
#define SPI_MOSI SPI0_MOSI
#define SPI_MISO SPI0_MISO
#define SPI_CE0N SPI0_CE0N
#define SPI_CE1N SPI0_CE1N

#define SPI_CLK_DIVIDE_MASK 0xFFFF
#define SPI_CLK_DIVIDE_DEFAULT 0

#define SPI_SELECT_0 0x01
#define SPI_SELECT_1 0x02
#define SPI_SELECT_DEFAULT SPI_SELECT_0

#define SPI_ACTIVATE 1
#define SPI_DEACTIVATE 0

#define GPIO_ALTF0  0x0b100
#define SPI0_CS_CPOL                 0x00000008 ///< Clock Polarity
#define SPI0_CS_CPHA                 0x00000004 ///< Clock Phase

static void peri_set_bits(volatile uint32_t addr, uint32_t value, uint32_t mask) {
	uint32_t v = get32(addr);
	v = (v & ~mask) | (value & mask);
	put32(addr, v);
	put32(addr, v);
}

void spi_init(int32_t clk_divide) {
	uint32_t a = get32(SPI_ENABLES);
	a |= 1;
	put32(SPI_ENABLES, a);

	/* setup spi pins (ALTF0) */
	gpio_config(SPI_SCLK, GPIO_ALTF0);
	gpio_config(SPI_MOSI, GPIO_ALTF0);
	gpio_config(SPI_MISO, GPIO_ALTF0);
	gpio_config(SPI_CE0N, GPIO_ALTF0);
	gpio_config(SPI_CE1N, GPIO_ALTF0);

	put32(SPI_CS_REG, 0);
	put32(SPI_CS_REG, 0);

	uint32_t data = SPI_CNTL_CLMASK; /* clear both rx/tx fifo */
	/* clear spi fifo */
	put32(SPI_CS_REG, data);
	/* set largest clock divider */
	clk_divide &= SPI_CLK_DIVIDE_MASK; /* 16-bit value */
	put32(SPI_CLK_REG, clk_divide); /** 0=65536, power of 2, rounded down */

	uint32_t data_mode = 0; 
	peri_set_bits(SPI_CS_REG, data_mode << 2, SPI_CNTL_CPOL | SPI_CNTL_CPHA) ;
}

void spi_write(uint32_t data) {
	peri_set_bits(SPI_CS_REG, SPI_CNTL_CLMASK, SPI_CNTL_CLMASK);
	peri_set_bits(SPI_CS_REG, SPI_CNTL_TRXACT, SPI_CNTL_TRXACT);
	/* wait if fifo is full */
	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDATA));
	/* write a byte */
	put32(SPI_FIFO_REG, data&0xff);
	/* wait until done */
	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDONE));
	peri_set_bits(SPI_CS_REG, 0, SPI_CNTL_TRXACT);
}

uint32_t spi_transfer(uint32_t data) {
	peri_set_bits(SPI_CS_REG, SPI_CNTL_CLMASK, SPI_CNTL_CLMASK);
	peri_set_bits(SPI_CS_REG, SPI_CNTL_TRXACT, SPI_CNTL_TRXACT);
	/* wait if fifo is full */
	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDATA));
	/* write a byte */
	put32(SPI_FIFO_REG, data&0xff);
	/* wait until done */
	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDONE));
	/* should get a byte? */
	while (!(get32(SPI_CS_REG)&SPI_STAT_RXDATA));
	/* read a byte */
	uint32_t r = get32(SPI_FIFO_REG)&0xff;
	peri_set_bits(SPI_CS_REG, 0, SPI_CNTL_TRXACT);
	return r;
}
