#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include <stdio.h>
#include <string.h>

#include "serial.h"
#include "i2c.h"

void i2c_init(void) {
	/*Prescaler set to 1 */
	TWSR = 0x00; 

	/*
	SCL frequency = CPU /(16 + 2(TWBR) ⋅ PrescalerValue)
	SCL = 100KHz
 
	TWBR = ((16 000 000÷100 000)-16)÷(2×1) ->
	= 72 ->
	= 0x48
	*/
	TWBR = 0x48;
		/*
	SCL: 2-wire Serial Interface Clock. 
	When the TWEN bit in TWCR is set (one) to enable the 2- wire Serial Interface, 
	pin PC5 is disconnected from the port and 
	becomes the Serial Clock I/O pin for the 2-wire Serial Interface. 
	In this mode, there is a spike filter on the pin to suppress spikes 
	shorter than 50 ns on the input signal, and the pin is driven by an 
	open drain driver with slew-rate limitation.
	*/
	TWCR = (1 << TWEN);
}

void i2c_meaningful_status(uint8_t status) {
	switch (status) {
		case 0x08: // START transmitted, proceed to load SLA+W/R
			printf_P(PSTR("START\n"));
			break;
		case 0x10: // repeated START transmitted, proceed to load SLA+W/R
			printf_P(PSTR("RESTART\n"));
			break;
		case 0x38: // NAK or DATA ARBITRATION LOST
			printf_P(PSTR("NOARB/NAK\n"));
			break;
		// MASTER TRANSMIT
		case 0x18: // SLA+W transmitted, ACK received
			printf_P(PSTR("MT SLA+W, ACK\n"));
			break;
		case 0x20: // SLA+W transmitted, NAK received
			printf_P(PSTR("MT SLA+W, NAK\n"));
				break;
		case 0x28: // DATA transmitted, ACK received
			printf_P(PSTR("MT DATA+W, ACK\n"));
			break;
		case 0x30: // DATA transmitted, NAK received
			printf_P(PSTR("MT DATA+W, NAK\n"));
			break;
		// MASTER RECEIVE
		case 0x40: // SLA+R transmitted, ACK received
			printf_P(PSTR("MR SLA+R, ACK\n"));
			break;
		case 0x48: // SLA+R transmitted, NAK received
			printf_P(PSTR("MR SLA+R, NAK\n"));
			break;
		case 0x50: // DATA received, ACK sent
			printf_P(PSTR("MR DATA+R, ACK\n"));
			break;
		case 0x58: // DATA received, NAK sent
			printf_P(PSTR("MR DATA+R, NAK\n"));
			break;
		default:
			printf_P(PSTR("N/A %02X\n"), status);
			break;
	}
}

inline void i2c_start() {
	/*
	TWCRn.TWEN must be written to '1' to enable the 2-wire Serial Interface
 	TWCRn.TWSTA must be written to '1' to transmit a START condition
 	TWCRn.TWINT must be cleared by writing a '1' to it.
	*/
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); 

	/* Wait for TWINT Flag set. This indicates that the START condition has been transmitted. */
	while (!(TWCR & (1 << TWINT)));
}

inline void i2c_stop() {
	/* Transmit STOP condition. */
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

	/* Wait for TWSTO Flag set. This indicates that the STOP condition has been transmitted. */
	while ((TWCR & (1 << TWSTO)));
}

inline uint8_t i2c_get_status(void) {
	uint8_t status;
	/* Read TWSR status register */
	status = TWSR & 0xF8;

	return status;
}

inline void i2c_xmit_addr(uint8_t address, uint8_t rw) {
	/*
	The TWDRn contains the address or data bytes to be transmitted, or the address or data bytes received  
	Set TWDR to Mask the first seven bits to adress and the last bit to read or write 1 | 0 */
	TWDR = (address & 0xfe) | (rw & 0x01);

	/* Perform operation by clearing TWINT bit and trigger enable:TWEN bit*/
	TWCR = (1 << TWINT) | (1 << TWEN);

	/* Wait until TWINT flag is set and operation is completed */
	while (!(TWCR & (1 << TWINT)));
}

inline void i2c_xmit_byte(uint8_t data) {
	/* Set TWDR to contain the data byte to be transmitted */
	TWDR = data;

	/* Perform operation by enabling TWINT and TWEN bits*/
	TWCR = (1 << TWINT) | (1 << TWEN);

	/* Wait until TWINT flag is set and operation is completed */
	while (!(TWCR & (1 << TWINT)));
}

/* reading byte with ack enabled tells eeprom to send ack and that i2c wants to continue reading*/
inline uint8_t i2c_read_ACK() {
	/*  enable ACK bit in TWCR */
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	/* Wait until TWINT flag is set and operation is completed */
	while (!(TWCR & (1 << TWINT)));

	return TWDR;
}

/* reading byte without ack tells eeprom that this is the last byte to be read*/
inline uint8_t i2c_read_NAK() {
	TWCR = (1 << TWINT) | (1 << TWEN);

	while (!(TWCR & (1 << TWINT)));

	return TWDR;
}

inline void eeprom_wait_until_write_complete() {
	/* run start sequence, set start adress */
	/* while SLA+W transmitted, ACK received is not true*/
	while (i2c_get_status() != 0x18) {
		i2c_start();
		i2c_xmit_addr(EEPROM_ADDR, WRITE);
	}
}

/* read one byte from i2c 
* Send start command to start communication with i2c
* tell i2c which device adress to communicate with 
	(using write bit to write adress)
* tell which adress on device to read from
* send a new start command and the device adress with READ bit
* read one byte with NAK 
* send stop command to i2c
* return the recieved byte 
*/
uint8_t eeprom_read_byte(uint8_t addr) {
	uint8_t readByte;

	i2c_start();
	i2c_xmit_addr(EEPROM_ADDR, WRITE);
	i2c_xmit_byte(addr);

	i2c_start();
	i2c_xmit_addr(EEPROM_ADDR, READ);
	readByte = i2c_read_NAK();

	i2c_stop();

	return readByte;
}

/* Send one byte to i2c
* run start sequence and then tell i2c which i2c-adress to use
* transmit the adress in the eeprom to be written
* transmit the byte to be written
* stop transmittion and the check status for success
*/
void eeprom_write_byte(uint8_t addr, uint8_t data) {
	i2c_start();
	i2c_xmit_addr(EEPROM_ADDR, WRITE);

	i2c_xmit_byte(addr);
	i2c_xmit_byte(data);

	i2c_stop();
	eeprom_wait_until_write_complete();
}

/* Page writing to eeprom allows for 8 bytes to be written in the same transmission
	eeprom internal buffer can store 8 bytes therefor writing past 8 bytes 
	will wrap around and start at the beginning of the page
	therefor the FOR loop will only handle the 8 first bits in data
	other functions need to handle that write page only receives 8 bytes
	*/
void eeprom_write_page(uint8_t addr, char data[8]) {
	i2c_start();
	i2c_xmit_addr(EEPROM_ADDR, WRITE);
	i2c_xmit_byte(addr);
	for(int i = 0; i <8; i++){
		i2c_xmit_byte(data[i]);
	}
	i2c_stop();
	eeprom_wait_until_write_complete();
}

/*
This function handles a string to be written to eeprom by dividing the string
into even portions of 8 and using write page to send to eeprom.
Bytes that are not evenly divedable with 8 are written using eeprom_write_byte
*/
void eeprom_sequential_write(uint8_t addr, char *data, size_t length){
	uint8_t singleBytes = length % 8; 				  //number for bytes in the string not evenly divadable with 8
	uint8_t numberOfPages = ((length-singleBytes)/8); //how many pages should be written
	uint8_t pageNumber = 0; 						  //start number for pages		
	char word[8];									  //buffer for page to be written

	if(numberOfPages > 0){							  //check if there is any pages
		int j = 0;		
		for(int i = 0; i <= length; i++){			  //iterate over data with the size given in function call
			word[j]=data[i];						  //set the buffer(word) to the next letter in data
			if(j == 7 ){							  //every 8 bytes a new page should be written to eeprom
				/* adress is calculated from the number of pages and 8 bytes forward*/
				eeprom_write_page(addr+(pageNumber*8), word); 
				pageNumber++;						  // increment the pagenumber for the next time a page should be written
				j = 0;								  // reset j to start at the beginning of the buffer(word)
			}else {
				j++;								  //if its not a page write, increment j to write to next position in the buffer(data)
			}
		}
	}
	
	if(singleBytes > 0){							  //check if there are any singleBytes to be written
		for(int s = 0; s <= singleBytes; s++){		  //iterate over the number of bytes
			int index = s+(length-singleBytes);		  //calculate what part of data should be written to eeprom
			eeprom_write_byte(addr+(pageNumber*8)+s, data[index]);
		}
	}
}

/* 
   read a given size of eeprom in one go.
   reading bytes with i2c_read_ACK allows for multiple bytes to be received
   without the need to stop and start over for each byte
   last byte is read with i2c_read_NAK and the a stop condition is sent.
*/
void eeprom_sequential_read(char *buf, uint8_t start_addr, uint8_t len) {
	i2c_start();
	i2c_xmit_addr(EEPROM_ADDR, 0);
	i2c_xmit_byte(start_addr);
	i2c_start();
	i2c_xmit_addr(EEPROM_ADDR, 1);

	for(int i = 0; i < len; i++){
		buf[i] = i2c_read_ACK();
	}
	buf[len] = i2c_read_NAK();
	i2c_stop();
}
