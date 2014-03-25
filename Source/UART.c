/*
 * uart.c
 *
 *  Created on: 27.10.2012
 *      Author: timo
 */
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <avr/io.h>

#define UART_BAUD_RATE  9600
#define UART_BAUD_REGISTERS  (((F_CPU / (UART_BAUD_RATE * 16UL))) - 1)

int UART_printCHAR(char character, FILE *stream)
{
   while ((UCSRA & (1 << UDRE)) == 0) {};

   UDR = character;

   return 0;
}

FILE uart_str = FDEV_SETUP_STREAM(UART_printCHAR, NULL, _FDEV_SETUP_RW);

void UART_init(void)
{
   UCSRB |= (1 << RXEN) | (1 << TXEN);
   UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1);

   UBRRL = UART_BAUD_REGISTERS;

   stdout = &uart_str;
}

char getch(void)
{
	while( !(UCSRA & _BV(RXC)) );

	return UDR;
}
