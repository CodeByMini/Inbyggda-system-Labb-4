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
