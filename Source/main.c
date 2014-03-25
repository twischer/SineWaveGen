#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define SINE_WAVE_DDR			DDRD
#define SINE_WAVE_PORT			PORTD
#define LOWER_HALF_WAVE_ENABLE	PD3
#define HIGHER_HALF_WAVE_ENABLE	PD4

#define HIGHER_OCR				OCR1AL
#define LOWER_OCR				OCR1BL



const uint8_t SINE_TABLE[] =  { 21,41,61,81,100,119,136,153,169,184,198,210,221,230,238,245,250,253,255,253,250,245,238,230,221,210,198,184,169,153,136,119,100,81,61,41,21, 0 };


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
}


int main(void)
{
	TCCR1A = (1<<COM1A1) | (1<<COM1B1) | (1<<WGM10);  // Clear on compare and phase correct 8-bit pwm
	TCCR1B = (0<<CS12) | (1<<CS10);     //PRE 1
	HIGHER_OCR = 0x00;
	LOWER_OCR = 0x00;
	TIMSK  = (1<<TOIE1); // enable overflow interrupt
	// enable PWM outputs
	DDRD |= (1<<PD5);
	DDRE |= (1<<PE2);
	
	SINE_WAVE_PORT = 0x00;
	SINE_WAVE_DDR |= (1<<LOWER_HALF_WAVE_ENABLE) | (1<<HIGHER_HALF_WAVE_ENABLE);
	sei();
	
	while (true)
	{
	}
	
	return 0;
}
