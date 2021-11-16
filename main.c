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

	i2c_init();
	uart_init();
	
	//char * text = "hej alla banditer som sitter har och skiter";
	char * text = "har du traffat jaques brels mindre kanda bror kjell brell gbg";
	//char * text = "hejpadigdingamlagalosch";
	//char * text = "hejja";

	eeprom_sequential_write(addr, text, strlen(text));

	char readBuffer[strlen(text)];
	eeprom_sequential_read(readBuffer, addr, strlen(text));
	printf_P(PSTR("From Main: %s\n\n"),readBuffer);

	printf_P(PSTR("Hexdump:"),readBuffer);
	for(int i = 0; i < strlen(text); i++){
		if(i % 16 == 0){
			printf_P(PSTR("\n"));
		}
		printf_P(PSTR("0x%x "),readBuffer[i]);
	}
	return 0;
}

