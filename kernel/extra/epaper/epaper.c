#include <dev/gpio.h>
#include <dev/spi.h>
#include <kernel/system.h>

#define EPD_RST_PIN      17
#define EPD_DC_PIN       25
#define EPD_CS_PIN       8
#define EPD_BUSY_PIN     24

#define EPD_WIDTH       104
#define EPD_HEIGHT      212

void epaper_reset(void) {
	gpio_write(EPD_RST_PIN, 1);
	_delay(200);
	gpio_write(EPD_RST_PIN, 0);
	_delay(200);
	gpio_write(EPD_RST_PIN, 1);
	_delay(20);
}
 
void epaper_cmd(uint8_t reg) {
	gpio_write(EPD_DC_PIN, 0);
	gpio_write(EPD_CS_PIN, 0);
	spi_write(reg);
	gpio_write(EPD_CS_PIN, 1);
}
 
void epaper_write(uint8_t data) {
	gpio_write(EPD_DC_PIN, 1);
	gpio_write(EPD_CS_PIN, 0);
	spi_write(data);
	gpio_write(EPD_CS_PIN, 1);
}

void epaper_wait(void) {
	while(gpio_read(EPD_BUSY_PIN) == 0) {
		_delay(100000);
	}
}

void epaper_on(void) {
	epaper_cmd(0x12);		 //DISPLAY REFRESH
	_delay(100000);
	epaper_wait();
}

void epaper_off(void) {
	epaper_cmd(0x02); // POWER_OFF
	epaper_wait();
	epaper_cmd(0x07); // DEEP_SLEEP
	epaper_write(0xA5); // check code
}

void epaper_init(void) {
	epaper_reset();

	epaper_cmd(0x06); // BOOSTER_SOFT_START
	epaper_write(0x17);
	epaper_write(0x17);
	epaper_write(0x17);

	epaper_cmd(0x04); // POWER_ON
	epaper_wait();

	epaper_cmd(0x00); // PANEL_SETTING
	epaper_write(0x8F);

	epaper_cmd(0x50); // VCOM_AND_DATA_INTERVAL_SETTING
	epaper_write(0xF0);
	epaper_cmd(0x61); // RESOLUTION_SETTING
	epaper_write(EPD_WIDTH); // width: 104
	epaper_write(EPD_HEIGHT >> 8); // height: 212
	epaper_write(EPD_HEIGHT & 0xFF);
}

void epaper_clear(void) {
	uint32_t w = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
	uint32_t h = EPD_HEIGHT;

	//send black data
	epaper_cmd(0x10);
	for (uint32_t j = 0; j < h; j++) {
		for (uint32_t i = 0; i < w; i++) {
			epaper_write(0xFF);
		}
	}
	epaper_cmd(0x92);

	//send red data
	epaper_cmd(0x13);
	for (uint32_t j = 0; j < h; j++) {
		for (uint32_t i = 0; i < w; i++) {
			epaper_write(0xFF);
		}
	}
	epaper_cmd(0x92);

	epaper_on();
}

#define SPI_CLK_DIVIDE_TEST 128
#define SPI_SELECT_0 0x01
#define SPI_SELECT_1 0x02

void epaper_test(void) {
	spi_init(SPI_CLK_DIVIDE_TEST);
	spi_select(SPI_SELECT_0);
	epaper_init();
	epaper_clear();
}
