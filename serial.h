#ifndef SERIAL_H_
#define SERIAL_H_

#define BAUDRATE 38400
#define UBRR (F_CPU/16/BAUDRATE-1)

void uart_init(void);

int uart_putchar(char chr, FILE *stream);
void uart_putchars(char chr);

char uart_getchar(void);

void uart_print_hex(uint8_t *buffer, uint8_t length);

#endif

