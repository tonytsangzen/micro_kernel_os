#ifndef SPI_H
#define SPI_H


void spi_init(int32_t clk_divide);
void spi_select(uint32_t which);
uint32_t spi_transfer(uint32_t data);
void spi_write(uint32_t data);

#endif
