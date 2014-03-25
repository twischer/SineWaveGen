#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "UART.h"

#define SINE_WAVE_DDR			DDRD
#define SINE_WAVE_PORT			PORTD
#define LOWER_HALF_WAVE_ENABLE	PD3
#define HIGHER_HALF_WAVE_ENABLE	PD6

#define HIGHER_OCR				OCR1AL
#define LOWER_OCR				OCR1BL



const uint8_t SINE_TABLE[] =  { 21,41,61,81,100,119,136,153,169,184,198,210,221,230,238,245,250,253,255,253,250,245,238,230,221,210,198,184,169,153,136,119,100,81,61,41,21, 0 };

static volatile uint8_t lastTimerCnt = 0;

ISR(TIMER1_OVF_vect)
{
	static uint8_t sineTableIndex = 0;
	sineTableIndex++;
	
	static bool higherHalfWave = true;
	
	if (sineTableIndex >= sizeof(SINE_TABLE))
	{
		higherHalfWave = !higherHalfWave;
		sineTableIndex = 0;
		
		if (higherHalfWave)
		{
			LOWER_OCR = 0x00;
			SINE_WAVE_PORT &= ~(1<<LOWER_HALF_WAVE_ENABLE);
			SINE_WAVE_PORT |= (1<<HIGHER_HALF_WAVE_ENABLE);
		}
		else
		{
			HIGHER_OCR = 0x00;
			SINE_WAVE_PORT &= ~(1<<HIGHER_HALF_WAVE_ENABLE);
			SINE_WAVE_PORT |= (1<<LOWER_HALF_WAVE_ENABLE);
		}
	}
	
	if (higherHalfWave)
		HIGHER_OCR = SINE_TABLE[sineTableIndex];
	else
		LOWER_OCR = SINE_TABLE[sineTableIndex];
	
	
	const uint8_t temp = TCNT1L;
	if (temp > lastTimerCnt)
		lastTimerCnt = temp;
}


int main(void)
{
	UART_init();
	
	TCCR1A = (1<<COM1A1) | (1<<COM1B1) | (1<<WGM10);  // Clear on compare and phase correct 8-bit pwm
	TCCR1B = (0<<CS12) | (1<<CS10);     //PRE 1
	HIGHER_OCR = 0x00;
	LOWER_OCR = 0x00;
	TIMSK  = (1<<TOIE1); // enable overflow interrupt
	// enable PWM outputs
	DDRD |= (1<<PD4) | (1<<PD5);
	
	SINE_WAVE_PORT = 0x00;
	SINE_WAVE_DDR |= (1<<LOWER_HALF_WAVE_ENABLE) | (1<<HIGHER_HALF_WAVE_ENABLE);
	sei();
	
	printf("Hello world!\r\n");
	while (true)
	{
		const uint8_t temp = lastTimerCnt;
		printf("Value: %d\r\n", temp);
//		PORTA ^= 0xFF;
		_delay_ms(500);
	}
	
	return 0;
}
