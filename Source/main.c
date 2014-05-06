#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


#define POS_WAVE_DDR				DDRA
#define POS_WAVE_PORT				PORTA
#define POS_WAVE_PIN				PA0

#define NEG_WAVE_DDR				DDRC
#define NEG_WAVE_PORT				PORTC
#define NEG_WAVE_PIN				PC4

#define POS_WAVE_OCR				OCR1AL
#define NEG_WAVE_OCR				OCR1BL

// The time out between a half bridge (in µs)
#define FET_TIMEOUT				3
// length of the v+ and v+ pulse (in ms)
#define PULSE_TIME				7
// length of the null pulse where both outputs are connected to gnd (in ms)
#define NULL_TIME				3



const uint8_t SINE_TABLE[] =  { 21,41,61,81,100,119,136,153,169,184,198,210,221,230,238,245,250,253,255,253,250,245,238,230,221,210,198,184,169,153,136,119,100,81,61,41,21, 0 };


ISR(TIMER1_OVF_vect)
{
	static uint8_t sineTableIndex = 0;
	sineTableIndex++;
	
	static bool higherHalfWave = true;
	
	if ( sineTableIndex >= (sizeof(SINE_TABLE) - 1) )
	{
		// disable all mosfets for one cycle
		// so not both mosfets of one half bridge can be enabled
		NEG_WAVE_OCR = 0x00;
		POS_WAVE_OCR = 0x00;
		NEG_WAVE_PORT &= ~(1<<NEG_WAVE_PIN);
		POS_WAVE_PORT &= ~(1<<POS_WAVE_PIN);
				
		if (sineTableIndex >= sizeof(SINE_TABLE))
		{
			higherHalfWave = !higherHalfWave;
			sineTableIndex = 0;
			
			if (higherHalfWave)
				POS_WAVE_PORT |= (1<<POS_WAVE_PIN);
			else
				NEG_WAVE_PORT |= (1<<NEG_WAVE_PIN);
		}
	}
	
	if (higherHalfWave)
		POS_WAVE_OCR = SINE_TABLE[sineTableIndex];
	else
		NEG_WAVE_OCR = SINE_TABLE[sineTableIndex];
}


int main(void)
{
	// open fets whcih are conected to v+
	POS_WAVE_PORT &= ~(1<<POS_WAVE_PIN);
	NEG_WAVE_PORT &= ~(1<<NEG_WAVE_PIN);
	POS_WAVE_DDR |= (1<<POS_WAVE_PIN);
	NEG_WAVE_DDR |= (1<<NEG_WAVE_PIN);
	
	// clear pwm outputs (open fets which are connected to gnd
	PORTD &= ~(1<<PD5);
	PORTE &= ~(1<<PE2);
	// enable PWM outputs
	DDRD |= (1<<PD5);		// Higher half wave PWM
	DDRE |= (1<<PE2);		// Lower half wave PWM
	
	
	// TODO read if from an jumper or switch
	const bool generateSineOutput = false;
	if (generateSineOutput)
	{
		// configure timer and pwm for sine wave output
		TCCR1A = (1<<COM1A1) | (1<<COM1B1) | (1<<WGM10);  // Clear on compare and phase correct 8-bit pwm
		TCCR1B = (0<<CS12) | (0<<CS11) | (1<<CS10);     //PRE 1
		TCCR1B = (1<<CS12) | (0<<CS11) | (1<<CS10);     //PRE 1024
		POS_WAVE_OCR = 0x00;
		NEG_WAVE_OCR = 0x00;
		TIMSK  = (1<<TOIE1); // enable overflow interrupt

		sei();
	}
	
	
	// otherwise generate a trapez
	// useful to minimize the switching time of the fets (more efficient)
	//				1	2	
	// 		NULL	-	-	!PC4	PD5
	// 		POS		+	-	!PE2	PA0
	// 		NULL	-	-	!PA0	PE2
	//		NEG		-	+	!PD5	PC4
	while (!generateSineOutput)
	{
		// TODO check every time if both outputs for the same half bridge are enabled at the same time
		// than switch off all outputs and stop the system
		// possible use a led for indecating this error
		
		// 	NULL	-	-	!PC4	PD5
		NEG_WAVE_PORT &= ~(1<<NEG_WAVE_PIN);
		_delay_us(FET_TIMEOUT);
		PORTD |= (1<<PD5);		// Positiv half wave PWM
		_delay_ms(NULL_TIME);
		
		
		// 	POS		+	-	!PE2	PA0
		PORTE &= ~(1<<PE2);		// Negativ half wave PWM
		_delay_us(FET_TIMEOUT);
		POS_WAVE_PORT |= (1<<POS_WAVE_PIN);
		_delay_ms(PULSE_TIME);
		
		
		// 	NULL	-	-	!PA0	PE2
		POS_WAVE_PORT &= ~(1<<POS_WAVE_PIN);
		_delay_us(FET_TIMEOUT);
		PORTE |= (1<<PE2);		// Negativ half wave PWM
		_delay_ms(NULL_TIME);
		
		
		//		NEG		-	+	!PD5	PC4
		PORTD &= ~(1<<PD5);		// Positiv half wave PWM
		_delay_us(FET_TIMEOUT);
		NEG_WAVE_PORT |= (1<<NEG_WAVE_PIN);
		_delay_ms(PULSE_TIME);
	}
	
	return 0;
}
