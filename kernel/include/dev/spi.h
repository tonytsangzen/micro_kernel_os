#ifndef SPI_H
#define SPI_H

#include <types.h>

void spi_init(int32_t clk_divide);
uint32_t spi_transfer(uint32_t data);
void spi_write(uint32_t data);
void spi_activate(uint32_t enable);
void spi_select(uint32_t which); 

#endif
