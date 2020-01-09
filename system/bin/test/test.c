#include <unistd.h>
#include <stdio.h>
#include <gpio.h>

void _delay(uint32_t cnt) {
	while(cnt > 0) cnt--;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	gpio_config(47, 1);//set 22 port to output mode , act led

	while(1) {
		gpio_write(47, 0);
		sleep(1);
		gpio_write(47, 1);
		sleep(1);
	}

  return 0;
}

