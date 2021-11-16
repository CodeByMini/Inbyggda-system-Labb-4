#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

#include "i2c.h"
#include "serial.h"

int main (void) {
	uint8_t addr = 0x10;
	char data[8] = "Daniel";

	i2c_init();
	uart_init();
	for (int i = 0; i <sizeof(data); i++){
		eeprom_write_byte(addr+i, data[i]);
	}
	
	char buffer[8];
	for(int i = 0; i <sizeof(data); i++){
		buffer[i] = eeprom_read_byte(addr+i);
	}
	printf_P(PSTR("%s\n"), buffer);
	
	return 0;
}

