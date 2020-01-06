#ifndef I2C_H
#define I2C_H

#include <types.h>

#define I2C_WAIT_DEFAULT 5

#define I2C_READ_STOP_DISABLE 0
#define I2C_READ_STOP_ENABLE 1
#define I2C_READ_STOP_DEFAULT I2C_READ_STOP_DISABLE

void i2c_init(uint32_t sda_gpio, uint32_t scl_gpio);
void i2c_set_wait_time(uint32_t wait_time);
void i2c_set_free_time(uint32_t free_time);
void i2c_set_read_stop(bool enable);

void i2c_putb(uint32_t addr, uint32_t regs, uint8_t data);
uint8_t i2c_getb(uint32_t addr, uint32_t regs);

uint32_t i2c_puts(uint32_t addr, uint32_t regs, uint8_t* pdat, uint32_t size);
uint32_t i2c_gets(uint32_t addr, uint32_t regs, uint8_t* pdat, uint32_t size);

#endif
