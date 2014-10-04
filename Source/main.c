#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include <util/delay.h>


#define BRIDGE1_HIGH_DDR		DDRA
#define BRIDGE1_HIGH_PORT		PORTA
#define BRIDGE1_HIGH_IN			PINA
#define BRIDGE1_HIGH_PIN		PA0

#define BRIDGE1_LOW_DDR			DDRE
#define BRIDGE1_LOW_PORT		PORTE
#define BRIDGE1_LOW_IN			PINE
#define BRIDGE1_LOW_PIN			PE2
#define BRIDGE1_LOW_OCR			OCR1BL


#define BRIDGE2_HIGH_DDR		DDRC
#define BRIDGE2_HIGH_PORT		PORTC
#define BRIDGE2_HIGH_IN			PINC
#define BRIDGE2_HIGH_PIN		PC4

#define BRIDGE2_LOW_DDR			DDRD
#define BRIDGE2_LOW_PORT		PORTD
#define BRIDGE2_LOW_IN			PIND
#define BRIDGE2_LOW_PIN			PD5
#define BRIDGE2_LOW_OCR			OCR1AL


#define OUTPUT_SWITCH_PORT		PORTD
#define OUTPUT_SWITCH_IN		PIND
#define OUTPUT_SWITCH_PIN		PD7




// The time out between a half bridge (in µs)
#define FET_TIMEOUT				3
// length of the v+ and v+ pulse (in ms)
#define PULSE_TIME				7
// length of the null pulse where both outputs are connected to gnd (in ms)
#define NULL_TIME				3



const uint8_t SINE_TABLE[] =  { 0, 21,41,61,81,100,119,136,153,169,184,198,210,221,230,238,245,250,253,255,253,250,245,238,230,221,210,198,184,169,153,136,119,100,81,61,41,21, 0 };


ISR(TIMER1_OVF_vect)
{
	static uint8_t sineTableIndex = 0;
	sineTableIndex++;
	
	static bool positiveHalfWave = true;
	
	if (sineTableIndex >= sizeof(SINE_TABLE))
	{
		positiveHalfWave = !positiveHalfWave;
		sineTableIndex = 0;
		
		// the deadband on each half bridge is given by the switch of in the sien table
		// (the 0 at the end)
		if (positiveHalfWave)
		{
			BRIDGE2_HIGH_PORT &= ~(1<<BRIDGE2_HIGH_PIN);
			BRIDGE1_HIGH_PORT |= (1<<BRIDGE1_HIGH_PIN);
		}
		else
		{
			BRIDGE1_HIGH_PORT &= ~(1<<BRIDGE1_HIGH_PIN);
			BRIDGE2_HIGH_PORT |= (1<<BRIDGE2_HIGH_PIN);
		}
	}
	
	if (positiveHalfWave)
	{
		BRIDGE2_LOW_OCR = SINE_TABLE[sineTableIndex];
		BRIDGE1_LOW_OCR = 0x00;
	}
	else
	{
		BRIDGE2_LOW_OCR = 0x00;
		BRIDGE1_LOW_OCR = SINE_TABLE[sineTableIndex];
	}
}


void switchOutputsOff(void)
{
	// open fets which are conected to v+
	BRIDGE1_HIGH_PORT &= ~(1<<BRIDGE1_HIGH_PIN);
	BRIDGE2_HIGH_PORT &= ~(1<<BRIDGE2_HIGH_PIN);
	
	// clear pwm outputs (open fets which are connected to gnd
	BRIDGE1_LOW_PORT &= ~(1<<BRIDGE1_LOW_PIN);
	BRIDGE2_LOW_PORT &= ~(1<<BRIDGE2_LOW_PIN);
	
	// disable timer and pwm to get control with the port registers to the output pins
	TCCR1A = 0x00;
	TCCR1B = 0x00;
}


void enterSafeMode(void)
{
	switchOutputsOff();
	
	// switch off watchdog to realy wait for reset
	wdt_disable();
	
	// wait for reset
	while (true)
	{
	}
}


void checkOutputConflicts(void)
{
	// switch all outputs off and wait for an avr reset
	// if both outputs of the same half bridge are active at the same time
	const uint8_t bridge1HighOut = BRIDGE1_HIGH_IN;
	const uint8_t bridge1LowOut = BRIDGE1_LOW_IN;
	if (  (bridge1HighOut & (1<<BRIDGE1_HIGH_PIN)) && (bridge1LowOut & (1<<BRIDGE1_LOW_PIN))  )
		enterSafeMode();
	
	const uint8_t bridge2HighOut = BRIDGE2_HIGH_IN;
	const uint8_t bridge2LowOut = BRIDGE2_LOW_IN;
	if (  (bridge2HighOut & (1<<BRIDGE2_HIGH_PIN)) && (bridge2LowOut & (1<<BRIDGE2_LOW_PIN))  )
		enterSafeMode();
	
	// reset watchdog timer
	wdt_reset();
}


void sineGenerator(void)
{
	// configure timer and pwm for sine wave output
	BRIDGE2_LOW_OCR = 0x00;
	BRIDGE1_LOW_OCR = 0x00;
	TCCR1A = (1<<COM1A1) | (1<<COM1B1) | (1<<WGM10);  // Clear on compare and phase correct 8-bit pwm
	TCCR1B = (0<<CS12) | (0<<CS11) | (1<<CS10);     //PRE 1
	TIMSK  = (1<<TOIE1); // enable overflow interrupt
	
	sei();
	
	DDRB = 0xFF;
	
	while(true)
	{
		checkOutputConflicts();
	}
}


void trapezeGenerator(void)
{	
	// otherwise generate a trapez
	// useful to minimize the switching time of the fets (more efficient)
	//				1	2	
	// 		NULL	-	-	!PC4	PD5
	// 		POS		+	-	!PE2	PA0
	// 		NULL	-	-	!PA0	PE2
	//		NEG		-	+	!PD5	PC4
	while (true)
	{
		// 	NULL	-	-	!PC4	PD5
		BRIDGE2_HIGH_PORT &= ~(1<<BRIDGE2_HIGH_PIN);
		_delay_us(FET_TIMEOUT);
		BRIDGE2_LOW_PORT |= (1<<BRIDGE2_LOW_PIN);		// Positiv half wave PWM
		
		checkOutputConflicts();
		_delay_ms(NULL_TIME);
		checkOutputConflicts();
		
		
		// 	POS		+	-	!PE2	PA0
		BRIDGE1_LOW_PORT &= ~(1<<BRIDGE1_LOW_PIN);		// Negativ half wave PWM
		_delay_us(FET_TIMEOUT);
		BRIDGE1_HIGH_PORT |= (1<<BRIDGE1_HIGH_PIN);
		
		checkOutputConflicts();
		_delay_ms(PULSE_TIME);
		checkOutputConflicts();
		
		
		// 	NULL	-	-	!PA0	PE2
		BRIDGE1_HIGH_PORT &= ~(1<<BRIDGE1_HIGH_PIN);
		_delay_us(FET_TIMEOUT);
		BRIDGE1_LOW_PORT |= (1<<BRIDGE1_LOW_PIN);		// Negativ half wave PWM
		
		checkOutputConflicts();
		_delay_ms(NULL_TIME);
		checkOutputConflicts();
		
		
		//		NEG		-	+	!PD5	PC4
		BRIDGE2_LOW_PORT &= ~(1<<BRIDGE2_LOW_PIN);		// Positiv half wave PWM
		_delay_us(FET_TIMEOUT);
		BRIDGE2_HIGH_PORT |= (1<<BRIDGE2_HIGH_PIN);
		
		checkOutputConflicts();
		_delay_ms(PULSE_TIME);
		checkOutputConflicts();
	}
}


int main(void)
{
	switchOutputsOff();
	
	// enable pull up for output switch input
	OUTPUT_SWITCH_PORT |= (1<<OUTPUT_SWITCH_PIN);
	
	BRIDGE1_HIGH_DDR |= (1<<BRIDGE1_HIGH_PIN);
	BRIDGE2_HIGH_DDR |= (1<<BRIDGE2_HIGH_PIN);
	
	// enable PWM outputs
	DDRD |= (1<<PD5);		// Higher half wave PWM
	DDRE |= (1<<PE2);		// Lower half wave PWM
	
	// enable watchdog timer
	wdt_enable(WDTO_15MS);
	
	
	// read output configuration from an jumper or switch
	const bool generateSineOutput = (OUTPUT_SWITCH_IN & (1<<OUTPUT_SWITCH_PIN));
	if (generateSineOutput)
		sineGenerator();
	else
		trapezeGenerator();
	
	return 0;
}
