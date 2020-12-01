#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdlib.h>
#include <string.h>

EMPTY_INTERRUPT(TIMER0_COMPA_vect)

static inline void usart_transmit(unsigned char data);

uint32_t adc_result;
ISR(ADC_vect)
{
    adc_result = ADCL;
   	adc_result |= ((uint32_t)ADCH << 8);
}

#define F_CPU 128000  // 128kHz

char adc_str[5];

static inline uint32_t read_adc(unsigned char pin)
{
    TIMSK0 &= (0xff ^ (0x01 << OCIE0A));	// Disable timer interrupt

    ADMUXA = pin;							// ADC<pin> used as input
    ADCSRA = 0x88;							// ADC enabled, ADC interrupts enabled
    ADCSRA |= (0x01 << ADSC);				// Start ADC

    sleep_mode();							// Wait until ADC interrupt

    //ADCSRA &= (0xff ^ (0x01 << ADSC));		// Stop ADC
    ADCSRA = 0x00;							// ADC disabled
    TIMSK0 |= (0x01 << OCIE0A);				// Re-enable timer interrupt

    return adc_result;
}

static inline void usart_init(unsigned baud)
{
	// Violates the datasheet instructions
	// by not subtracting 1, but works
	baud = F_CPU / (16*baud); // - 1;

	UBRR0H = (unsigned char)(baud >> 8);
	UBRR0L = (unsigned char)baud;

	UCSR0A = 0;
	UCSR0B = (1 << TXEN0);
	UCSR0C = (1 << USBS0) | (3 << UCSZ00);
}

static inline void usart_transmit(unsigned char data)
{
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

static inline void usart_println(char *str, unsigned len)
{
	for (unsigned i = 0; i < len; i++)
		usart_transmit(str[i]);
	usart_transmit('\r');
	usart_transmit('\n');
}

static inline void setup_system()
{
    cli();

    DDRA = (1 << 3) | (1 << 7);

    wdt_disable();

    TCCR0A = 0x02;              // CTC mode for timer0
    TIMSK0 = (0x01 << OCIE0A);  // Enable compare interrupt
    TCCR0B = (0x5 << CS00);     // Select clock mode; clock prescaler clk/1024
    OCR0A = 128;                // Compare value; 128/(128kHz/1024) = 1s

	ADMUXB = 0x00;		// Vcc as reference voltage for ADC, gain=1

    sei();
}

int main(void)
{
	setup_system();
	usart_init(1200);

	static unsigned threshold = 128;

	while (1)
	{
		for (int i = 0; i < 8; i++)
			sleep_mode();

		PORTA = (1 << 7);
		uint32_t analog = read_adc(0);
		itoa(analog, adc_str, 10);
		usart_println(adc_str, strlen(adc_str));

		if (analog < threshold) {
			PORTA = (1 << 3);
			for (int i = 0; i < 2; i++)
				sleep_mode();
		}
		PORTA = 0;
	}
}

