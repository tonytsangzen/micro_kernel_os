#include "gpio.h"
#include "spi.h"
#include <dev/sd.h>
#include <kernel/system.h>
#include <mm/mmu.h>
/*----------------------------------------------------------------------------*/
#define SPI_CLK_DIVIDE_TEST 1024
/*
  512      488 kHz
  1024      244 kHz
*/
/*----------------------------------------------------------------------------*/
#define SDCARD_CMD00 0x00
#define SDCARD_CMD00_CRC 0x95
#define SDCARD_CMD01 0x01
#define SDCARD_CMD01_CRC 0xF9
#define SDCARD_CMD08 0x08
#define SDCARD_CMD08_ARG 0x000001AA
#define SDCARD_CMD08_CRC 0x87
#define SDCARD_CMD12 0x0C
#define SDCARD_CMD16 0x10
#define SDCARD_CMD17 0x11
#define SDCARD_CMD41 0x29
/** CMD41 arg/crc assumes HCS (high capacity?), else 0x00000000 & 0xE5 */
#define SDCARD_CMD41_ARG 0x40000000
#define SDCARD_CMD41_CRC 0x77
#define SDCARD_CMD55 0x37
#define SDCARD_CMD55_CRC 0x65
#define SDCARD_CMD59 0x3B
/*----------------------------------------------------------------------------*/
#define SDCARD_SWRESET SDCARD_CMD00
#define SDCARD_DOINIT SDCARD_CMD01
#define SDCARD_STOPTX SDCARD_CMD12
#define SDCARD_CHBSIZE SDCARD_CMD16
#define SDCARD_RDBLOCK SDCARD_CMD17
#define SDCARD_APPCMD SDCARD_CMD55
#define SDCARD_NOCRC SDCARD_CMD59
/*----------------------------------------------------------------------------*/
#define SDCARD_ARG_NONE_ 0x00000000
#define SDCARD_DUMMY_DATA 0xFF
#define SDCARD_DUMMY_CRC SDCARD_DUMMY_DATA
/*----------------------------------------------------------------------------*/
#define SDCARD_MMC_MASK 0xC0
#define SDCARD_MMC_CMD_ 0x40
#define SDCARD_SECTOR_SIZE 512
/*----------------------------------------------------------------------------*/
#define SDCARD_RESP_SUCCESS 0x00
#define SDCARD_RESP_R1_IDLE 0x01
#define SDCARD_RESP_ILLEGAL 0x05
#define SDCARD_RESP_INVALID 0xFF
#define SDCARD_RESP_WAIT_STEP 100
/*----------------------------------------------------------------------------*/
#define RESP_R1_IDLE_STATE  0x01
#define RESP_R1_ERASE_RESET 0x02
#define RESP_R1_ILLEGAL_CMD 0x04
#define RESP_R1_CMD_CRC_ERR 0x08
#define RESP_R1_ERASESQ_ERR 0x10
#define RESP_R1_ADDRESS_ERR 0x20
#define RESP_R1_PARAM_ERROR 0x40
/*----------------------------------------------------------------------------*/
#define SDCARD_FLUSH_R1 1
#define SDCARD_FLUSH_R7 4
/*----------------------------------------------------------------------------*/
int32_t sd_init(dev_t* dev)
{
	(void)dev;
	/** initialize gpio */
	gpio_init();
	spi_init(SPI_CLK_DIVIDE_TEST);
	spi_select(SPI_SELECT_1);

	int32_t loop;
	/* sdc/mmc init flow (spi) */
	_delay_msec(1000); /* wait at least 1ms */
	/* send 74 dummy clock cycles? 10 x 8-bit data?*/
	spi_activate(SPI_ACTIVATE);
	for (loop=0;loop<10;loop++) {
		spi_transfer(SDCARD_DUMMY_DATA);
	}
	spi_activate(SPI_DEACTIVATE);
	return 0;
}
/*----------------------------------------------------------------------------*/
uint32_t sd_command(int32_t cmd, uint32_t arg, int32_t crc)
{
	uint32_t res, cnt = SDCARD_RESP_WAIT_STEP;
	spi_activate(SPI_ACTIVATE);
	spi_transfer(cmd|SDCARD_MMC_CMD_);
	spi_transfer((arg>>24)&0xff);
	spi_transfer((arg>>16)&0xff);
	spi_transfer((arg>>8)&0xff);
	spi_transfer((arg)&0xff);
	spi_transfer(crc);
	do
	{
		/* wait card to be ready  */
		if ((res=spi_transfer(SDCARD_DUMMY_DATA))!=SDCARD_RESP_INVALID)
			break;
	}
	while(--cnt>0);
	spi_activate(SPI_DEACTIVATE);
	return res;
}
/*----------------------------------------------------------------------------*/
void sd_doflush(int32_t count,unsigned char *pbuff)
{
	int32_t loop, test;
	spi_activate(SPI_ACTIVATE);
	for (loop=0;loop<count;loop++)
	{
		test = (int) spi_transfer(SDCARD_DUMMY_DATA);
		if (pbuff) pbuff[loop] = (unsigned char) (test&0xff);
	}
	spi_activate(SPI_DEACTIVATE);
}
/*----------------------------------------------------------------------------*/
uint32_t sd_read_block(uint32_t sector, unsigned char* buffer)
{
	int32_t loop;
	uint32_t res, cnt = SDCARD_RESP_WAIT_STEP;
	do
	{
		/* sector size = 512, checksum should already be disabled? */
		res = sd_command(SDCARD_RDBLOCK,sector<<9,SDCARD_DUMMY_CRC);
		sd_doflush(SDCARD_FLUSH_R1,0x0);
		if (res==0x00) break;
	}
	while(--cnt>0);
	if (cnt == 0) 
		return res;
	/* wait?? */
	spi_activate(SPI_ACTIVATE);
	cnt = SDCARD_RESP_WAIT_STEP;
	do
	{
		if ((res=spi_transfer(SDCARD_DUMMY_DATA))==0xFE) break;
	}
	while(--cnt>0);
	if (cnt == 0) 
		return res;
	/* get the data */
	for (loop=0;loop<SDCARD_SECTOR_SIZE;loop++)
		buffer[loop] = spi_transfer(SDCARD_DUMMY_DATA);
	/* 16-bit dummy crc? */
	spi_transfer(SDCARD_DUMMY_CRC);
	spi_transfer(SDCARD_DUMMY_CRC);
	spi_activate(SPI_DEACTIVATE);
	return SDCARD_SECTOR_SIZE;
}

void sd_dev_handle(dev_t* dev) {
	(void)dev;
}

static int32_t _block = 0;

int32_t sd_dev_read(dev_t* dev, int32_t block) {
	(void)dev;
	_block = block;
	return 0;
}

int32_t sd_dev_read_done(dev_t* dev, void* buf) {
	(void)dev;
	if(sd_read_block(_block, (unsigned char*)buf) == 0)
		return -1;
	return 0;
}

int32_t sd_dev_write(dev_t* dev, int32_t block, const void* buf) { //TODO
	(void)dev;
	(void)block;
	(void)buf;
	return 0;
}

int32_t sd_dev_write_done(dev_t* dev) { //TODO
	(void)dev;
	return 0;
}
