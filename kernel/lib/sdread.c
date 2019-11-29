#include <sdread.h>
#include <dev/sd.h>
#include <dev/device.h>

int sd_read(int block, void* buf) {
	dev_t* dev = get_dev(DEV_SD);
	if(dev == NULL) 
		return -1;

	if(dev_block_read(dev, block) != 0)
		return -1;

	while(1) {
		if(dev_block_read_done(dev, buf)  == 0)
			break;
	}
	return 0;
}

