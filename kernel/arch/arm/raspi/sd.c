#include "gpio_arch.h"
#include <dev/sd.h>
#include <kernel/system.h>
#include <kernel/proc.h>
#include <basic_math.h>
#include <mm/mmu.h>
#include <kstring.h>

#define SD_OK                0
#define SD_TIMEOUT          -1
#define SD_ERROR            -2

#define EMMC_ARG2           ((volatile uint32_t*)(_mmio_base+0x00300000))
#define EMMC_BLKSIZECNT     ((volatile uint32_t*)(_mmio_base+0x00300004))
#define EMMC_ARG1           ((volatile uint32_t*)(_mmio_base+0x00300008))
#define EMMC_CMDTM          ((volatile uint32_t*)(_mmio_base+0x0030000C))
#define EMMC_RESP0          ((volatile uint32_t*)(_mmio_base+0x00300010))
#define EMMC_RESP1          ((volatile uint32_t*)(_mmio_base+0x00300014))
#define EMMC_RESP2          ((volatile uint32_t*)(_mmio_base+0x00300018))
#define EMMC_RESP3          ((volatile uint32_t*)(_mmio_base+0x0030001C))
#define EMMC_DATA           ((volatile uint32_t*)(_mmio_base+0x00300020))
#define EMMC_STATUS         ((volatile uint32_t*)(_mmio_base+0x00300024))
#define EMMC_CONTROL0       ((volatile uint32_t*)(_mmio_base+0x00300028))
#define EMMC_CONTROL1       ((volatile uint32_t*)(_mmio_base+0x0030002C))
#define EMMC_INTERRUPT      ((volatile uint32_t*)(_mmio_base+0x00300030))
#define EMMC_INT_MASK       ((volatile uint32_t*)(_mmio_base+0x00300034))
#define EMMC_INT_EN         ((volatile uint32_t*)(_mmio_base+0x00300038))
#define EMMC_CONTROL2       ((volatile uint32_t*)(_mmio_base+0x0030003C))
#define EMMC_SLOTISR_VER    ((volatile uint32_t*)(_mmio_base+0x003000FC))

// command flags
#define CMD_NEED_APP        0x80000000
#define CMD_RSPNS_48        0x00020000
#define CMD_ERRORS_MASK     0xfff9c004
#define CMD_RCA_MASK        0xffff0000

// COMMANDs
#define CMD_GO_IDLE         0x00000000
#define CMD_ALL_SEND_CID    0x02010000
#define CMD_ALL_SEND_CSD    0x09010000
#define CMD_SEND_REL_ADDR   0x03020000
#define CMD_CARD_SELECT     0x07030000
#define CMD_SEND_IF_COND    0x08020000
#define CMD_STOP_TRANS      0x0C030000
#define CMD_READ_SINGLE     0x11220010
#define CMD_READ_MULTI      0x12220032
#define CMD_SET_BLOCKCNT    0x17020000
#define CMD_WRITE_SINGLE    0x18220010
#define CMD_WRITE_MULTI     0x19220032
#define CMD_APP_CMD         0x37000000
#define CMD_SET_BUS_WIDTH   (0x06020000|CMD_NEED_APP)
#define CMD_SEND_OP_COND    (0x29020000|CMD_NEED_APP)
#define CMD_SEND_SCR        (0x33220010|CMD_NEED_APP)

// STATUS register settings
#define SR_READ_AVAILABLE   0x00000800
#define SR_WRITE_AVAILABLE  0x00000400
#define SR_DAT_INHIBIT      0x00000002
#define SR_CMD_INHIBIT      0x00000001
#define SR_APP_CMD          0x00000020

// INTERRUPT register settings
#define INT_DATA_TIMEOUT    0x00100000
#define INT_CMD_TIMEOUT     0x00010000
#define INT_READ_RDY        0x00000020
#define INT_WRITE_RDY       0x00000010
#define INT_DATA_DONE       0x00000002
#define INT_CMD_DONE        0x00000001

#define INT_ERROR_MASK      0x017E8000

// CONTROL register settings
#define C0_SPI_MODE_EN      0x00100000
#define C0_HCTL_HS_EN       0x00000004
#define C0_HCTL_DWITDH      0x00000002

#define C1_SRST_DATA        0x04000000
#define C1_SRST_CMD         0x02000000
#define C1_SRST_HC          0x01000000
#define C1_TOUNIT_DIS       0x000f0000
#define C1_TOUNIT_MAX       0x000e0000
#define C1_CLK_GENSEL       0x00000020
#define C1_CLK_EN           0x00000004
#define C1_CLK_STABLE       0x00000002
#define C1_CLK_INTLEN       0x00000001

// SLOTISR_VER values
#define HOST_SPEC_NUM       0x00ff0000
#define HOST_SPEC_NUM_SHIFT 16
#define HOST_SPEC_V3        2
#define HOST_SPEC_V2        1
#define HOST_SPEC_V1        0

// SCR flags
#define SCR_SD_BUS_WIDTH_4  0x00000400
#define SCR_SUPP_SET_BLKCNT 0x02000000
// added by my driver
#define SCR_SUPP_CCS        0x00000001

#define ACMD41_VOLTAGE      0x00ff8000
#define ACMD41_CMD_COMPLETE 0x80000000
#define ACMD41_CMD_CCS      0x40000000
#define ACMD41_ARG_HC       0x51ff8000

uint32_t sd_scr[2], sd_ocr, sd_rca, sd_hv;
int32_t sd_err;

// shared variables between SDC driver and interrupt handler
typedef struct {
	int32_t block;
	char rxbuf[SD_BLOCK_SIZE];
	char txbuf[SD_BLOCK_SIZE];
	char *rxbuf_index;
	const char *txbuf_index;
	uint32_t rxcount, txcount, rxdone, txdone;
} sd_t;

static sd_t _sdc;

static inline void wait_usec(uint32_t n) {
	while(n > 0) {
		_delay(10000);
		n--;
	}
}

/**
 * Wait for data or command ready
 */
static int32_t sd_status(uint32_t mask) {
	int32_t cnt = 1000000; 
	while((*EMMC_STATUS & mask) != 0 && (*EMMC_INTERRUPT & INT_ERROR_MASK) == 0 && cnt > 0)
		wait_usec(1);
	return (cnt <= 0 || (*EMMC_INTERRUPT & INT_ERROR_MASK)) ? SD_ERROR : SD_OK;
}

/**
 * Wait for interrupt
 */
static int32_t sd_int(uint32_t mask, int32_t wait) {
	uint32_t r, m = (mask | INT_ERROR_MASK);
	int32_t cnt = 10000; 
	while((*EMMC_INTERRUPT & m) == 0 && cnt--) {
		wait_usec(1);
		if(wait == 0)
			return -1;
	}

	r = *EMMC_INTERRUPT;
	if(cnt<=0 || (r & INT_CMD_TIMEOUT) != 0 || (r & INT_DATA_TIMEOUT) != 0) {
		*EMMC_INTERRUPT = r; 
		return SD_TIMEOUT;
	}
	else if((r & INT_ERROR_MASK) != 0) { 
		*EMMC_INTERRUPT = r;
		return SD_ERROR;
	}
	*EMMC_INTERRUPT = mask;
	return 0;
}

/**
 * Send a command
 */
static int32_t sd_cmd(uint32_t code, uint32_t arg) {
	int32_t r = 0;
	sd_err = SD_OK;
	if(code & CMD_NEED_APP) {
		r = sd_cmd(CMD_APP_CMD|(sd_rca?CMD_RSPNS_48:0), sd_rca);
		if(sd_rca != 0 && r != 0) {
			sd_err = SD_ERROR;
			return 0;
		}
		code &= ~CMD_NEED_APP;
	}
	if(sd_status(SR_CMD_INHIBIT)) { 
		sd_err = SD_TIMEOUT;
		return 0;
	}
	*EMMC_INTERRUPT = *EMMC_INTERRUPT; 
	*EMMC_ARG1 = arg;
	*EMMC_CMDTM = code;
	if(code == CMD_SEND_OP_COND)
		wait_usec(1000);
	else if(code==CMD_SEND_IF_COND || code==CMD_APP_CMD)
		wait_usec(100);

	if((r = sd_int(INT_CMD_DONE, 1))) {
		sd_err = r;
		return 0;
	}

	r = *EMMC_RESP0;
	if(code==CMD_GO_IDLE || code==CMD_APP_CMD)
		return 0; 
	else if(code==(CMD_APP_CMD|CMD_RSPNS_48)) {
		return r&SR_APP_CMD; 
	}
	else if(code==CMD_SEND_OP_COND)
		return r;
	else if(code==CMD_SEND_IF_COND)
		return r==(int32_t)arg? SD_OK : SD_ERROR; 
	else if(code==CMD_ALL_SEND_CID) {
		r |= *EMMC_RESP3;
		r |= *EMMC_RESP2;
		r |= *EMMC_RESP1;
		return r;
	} 
	else if(code == CMD_SEND_REL_ADDR) {
		sd_err = (((r&0x1fff))|((r&0x2000)<<6)|((r&0x4000)<<8)|((r&0x8000)<<8)) & CMD_ERRORS_MASK;
		return r & CMD_RCA_MASK;
	}
	return r & CMD_ERRORS_MASK;
}

/**
 * read a block from sd card and return the number of bytes read
 * returns 0 on error.
 */
static int32_t sd_read_sector(uint32_t lba) {
	if(sd_status(SR_DAT_INHIBIT)) {
		sd_err = SD_TIMEOUT;
		return -1;
	}

	*EMMC_BLKSIZECNT = (1 << 16) | 512;
	if((sd_scr[0] & SCR_SUPP_CCS) == 0) {
		sd_cmd(CMD_READ_SINGLE, (lba)*512);
		if(sd_err != 0)
			return -1;
	}
	return 0;
}

/**
 * write a block to the sd card and return the number of bytes written
 * returns 0 on error.
 */
static int32_t sd_write_sector(uint32_t lba, unsigned char *buffer, uint32_t num) {
	uint32_t r, d, c = 0;
	if(num < 1)
		num = 1;
	if(sd_status(SR_DAT_INHIBIT | SR_WRITE_AVAILABLE)) {
		sd_err = SD_TIMEOUT;
		return 0;
	}
	uint32_t *buf = (uint32_t *)buffer;
	if(sd_scr[0] & SCR_SUPP_CCS) {
		if(num > 1 && (sd_scr[0] & SCR_SUPP_SET_BLKCNT)) {
			sd_cmd(CMD_SET_BLOCKCNT,num);
			if(sd_err) 
				return 0;
		}
		*EMMC_BLKSIZECNT = (num << 16) | 512;
		sd_cmd(num == 1 ? CMD_WRITE_SINGLE : CMD_WRITE_MULTI,lba);
		if(sd_err)
			return 0;
	} 
	else {
		*EMMC_BLKSIZECNT = (1 << 16) | 512;
	}
	while( c < num ) {
		if(!(sd_scr[0] & SCR_SUPP_CCS)) {
			sd_cmd(CMD_WRITE_SINGLE,(lba+c)*512);
			if(sd_err) 
				return 0;
		}
		if((r = sd_int(INT_WRITE_RDY, 1))){
			sd_err = r;
			return 0;
		}
		for(d=0; d<128; d++) 
			*EMMC_DATA = buf[d];
		c++;
		buf+=128;
	}
	if((r = sd_int(INT_DATA_DONE, 1))) {
		sd_err = r;
		return 0;
	}
	if(num > 1 && (sd_scr[0] & SCR_SUPP_SET_BLKCNT) == 0 && (sd_scr[0] & SCR_SUPP_CCS) != 0) 
		sd_cmd(CMD_STOP_TRANS,0);
	return sd_err!=SD_OK || c!=num? 0 : num*512;
}

/**
 * set SD clock to frequency in Hz
 */
static int32_t sd_clk(uint32_t f) {
	uint32_t d, c=div_u32(41666666,f), x , s=32, h=0;
	int32_t cnt = 100000;
	while((*EMMC_STATUS & (SR_CMD_INHIBIT|SR_DAT_INHIBIT)) && cnt--) 
		wait_usec(1);
	if(cnt<=0) {
		return SD_ERROR;
	}

	*EMMC_CONTROL1 &= ~C1_CLK_EN;
	wait_usec(10);
	x=c-1;
	if(!x)
		s=0; 
	else {
		if(!(x & 0xffff0000u)) { x <<= 16; s -= 16; }
		if(!(x & 0xff000000u)) { x <<= 8;  s -= 8; }
		if(!(x & 0xf0000000u)) { x <<= 4;  s -= 4; }
		if(!(x & 0xc0000000u)) { x <<= 2;  s -= 2; }
		if(!(x & 0x80000000u)) { x <<= 1;  s -= 1; }
		if(s > 0) s--;
		if(s > 7) s = 7;
	}
	if(sd_hv > HOST_SPEC_V2) 
		d = c; 
	else
		d = (1<<s);
	if(d <= 2) {
		d = 2;
		s = 0;
	}
	if(sd_hv > HOST_SPEC_V2)
		h = (d&0x300)>>2;

	d = (((d&0x0ff)<<8)|h);
	*EMMC_CONTROL1 = (*EMMC_CONTROL1&0xffff003f) | d;
	_delay(1000000);
	*EMMC_CONTROL1 |= C1_CLK_EN;
	_delay(1000000);
	cnt=10000; 
	while(!(*EMMC_CONTROL1 & C1_CLK_STABLE) && cnt--)
		_delay(1000000);
	if(cnt<=0) {
		return SD_ERROR;
	}
	return SD_OK;
}

/**
 * initialize EMMC to read SDHC card
 */
int32_t sd_init(dev_t* dev) {
	(void)dev;
	_sdc.rxdone = 1;
	_sdc.txdone = 1;

	int64_t r, cnt, ccs = 0;
	// GPIO_IO_CD
	r = *GPIO_FSEL4;
	r &= ~(7<<(7*3));
	*GPIO_FSEL4 = r;
	*GPIO_PUD=2;
	_delay(150);
	
	*GPIO_PUDCLK1 = (1<<15);
	_delay(150);
	
	*GPIO_PUD = 0;
	*GPIO_PUDCLK1 = 0;
	r = *GPIO_HEN1;
	r |= 1<<15;
	*GPIO_HEN1 = r;

	// GPIO_IO_CLK, GPIO_IO_CMD
	r = *GPIO_FSEL4;
	r |= (7<<(8*3)) | (7<<(9*3));
	*GPIO_FSEL4 = r;
	*GPIO_PUD=2;
	_delay(150);
	*GPIO_PUDCLK1 = (1<<16)|(1<<17); 
	_delay(150);
	*GPIO_PUD = 0; 
	*GPIO_PUDCLK1 = 0;

	// GPIO_IO_DAT0, GPIO_IO_DAT1, GPIO_IO_DAT2, GPIO_IO_DAT3
	r = *GPIO_FSEL5;
	r |= (7<<(0*3)) | (7<<(1*3)) | (7<<(2*3)) | (7<<(3*3));
	*GPIO_FSEL5 = r;
	*GPIO_PUD = 2; 
	_delay(150);
	*GPIO_PUDCLK1 = (1<<18) | (1<<19) | (1<<20) | (1<<21);
	_delay(150);
	*GPIO_PUD = 0;
	*GPIO_PUDCLK1 = 0;

	sd_hv = (*EMMC_SLOTISR_VER & HOST_SPEC_NUM) >> HOST_SPEC_NUM_SHIFT;
	// Reset the card.
	*EMMC_CONTROL0 = 0;
	*EMMC_CONTROL1 |= C1_SRST_HC;
	cnt = 10000;
	do{
		wait_usec(10);
	} while((*EMMC_CONTROL1 & C1_SRST_HC) && cnt-- );

	if(cnt<=0)
		return SD_ERROR;

	*EMMC_CONTROL1 |= C1_CLK_INTLEN | C1_TOUNIT_MAX;
	wait_usec(10);
	// Set clock to setup frequency.
	if((r = sd_clk(400000)))
		return r;
	*EMMC_INT_EN   = 0xffffffff;
	*EMMC_INT_MASK = 0xffffffff;
	sd_scr[0] = sd_scr[1] = sd_rca = sd_err = 0;
	sd_cmd(CMD_GO_IDLE, 0);
	if(sd_err)
		return sd_err;

	sd_cmd(CMD_SEND_IF_COND, 0x000001AA);
	if(sd_err) 
		return sd_err;
	cnt = 6;
	r = 0;
	while(!(r&ACMD41_CMD_COMPLETE) && cnt--) {
		_delay(400);
		r = sd_cmd(CMD_SEND_OP_COND, ACMD41_ARG_HC);
		if((r & ACMD41_CMD_COMPLETE) &&
				(r & ACMD41_VOLTAGE) &&
				(r & ACMD41_CMD_CCS) &&
				(sd_err!=SD_TIMEOUT && sd_err!=SD_OK)) {
			return sd_err;
		}
	}

	if(!(r & ACMD41_CMD_COMPLETE) || !cnt ) 
		return SD_TIMEOUT;
	if(!(r & ACMD41_VOLTAGE))
		return SD_ERROR;
	if(r & ACMD41_CMD_CCS) 
		ccs = SCR_SUPP_CCS;
	sd_cmd(CMD_ALL_SEND_CID, 0);
	sd_rca = sd_cmd(CMD_SEND_REL_ADDR, 0);
	if(sd_err)
		return sd_err;
	sd_cmd(CMD_CARD_SELECT, sd_rca);
	if(sd_err)
		return sd_err;
	if(sd_status(SR_DAT_INHIBIT))
		return SD_TIMEOUT;
	*EMMC_BLKSIZECNT = (1<<16) | 8;
	if((r=sd_clk(25000000)))
		return r;

	/*
	sd_cmd(CMD_SEND_SCR, 0);
	if(sd_int(INT_READ_RDY, 1))
		return SD_TIMEOUT;

	r=0; cnt=100000; 
	while(r<2 && cnt) {
		if( *EMMC_STATUS & SR_READ_AVAILABLE )
			sd_scr[r++] = *EMMC_DATA;
		else
			wait_usec(1);
	}
	if(r != 2) 
		return SD_TIMEOUT;

	if(sd_scr[0] & SCR_SD_BUS_WIDTH_4) {
		sd_cmd(CMD_SET_BUS_WIDTH, sd_rca|2);
		if(sd_err)
			return sd_err;
		*EMMC_CONTROL0 |= C0_HCTL_DWITDH;
	}
	*/
	// add software flag
	sd_scr[0] &= ~SCR_SUPP_CCS;
	sd_scr[0] |= ccs;
	return SD_OK;
}

void sd_dev_handle(dev_t* dev) {
	if(_sdc.rxdone == 1)
		return;

	uint32_t d;
	if((sd_int(INT_READ_RDY, 0)) != 0) {
		return;
	}
	uint32_t* buf = (uint32_t*)_sdc.rxbuf_index;
	for(d=0; d<128; d++)
		buf[d] = *EMMC_DATA;

	_sdc.rxbuf_index += 512;
	_sdc.rxcount -= 512;
	proc_wakeup((uint32_t)dev);
	if(_sdc.rxcount == 0) {
		_sdc.rxdone = 1;
		sd_cmd(CMD_STOP_TRANS,0);
		return;
	}
	_sdc.block++;
	sd_read_sector(_sdc.block);
}

int32_t sd_dev_read(dev_t* dev, int32_t block) {
	(void)dev;
	if(_sdc.rxdone == 0)
		return -1;

	int32_t n = SD_BLOCK_SIZE/512;
	_sdc.block = block*n;
	_sdc.rxcount = SD_BLOCK_SIZE;
	_sdc.rxdone = 0;
	_sdc.rxbuf_index = _sdc.rxbuf;
	return sd_read_sector(_sdc.block);
}

int32_t sd_dev_read_done(dev_t* dev, void* buf) {
	(void)dev;
	sd_dev_handle(dev);
	if(_sdc.rxdone == 0)
		return -1;
	memcpy(buf, _sdc.rxbuf, SD_BLOCK_SIZE);
	return 0;
}

int32_t sd_dev_write(dev_t* dev, int32_t block, const void* buf) {
	(void)dev;
	int32_t n = SD_BLOCK_SIZE/512;
	if(sd_write_sector(block*n, (unsigned char*)buf, n) == 0)
		return -1;
	return 0;
}

int32_t sd_dev_write_done(dev_t* dev) {
	(void)dev;
	return 0;
}
