#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdlib.h>
#include <string.h>

static inline void usart_transmit(unsigned char data);
static inline void usart_println(char *str, unsigned len);
static inline uint32_t read_adc(unsigned char pin);

static unsigned threshold = 128;
static uint8_t measure = 0;

uint32_t adc_result;
char adc_str[5];

ISR(ADC_vect)
{
    adc_result = ADCL;
    adc_result |= ((uint32_t)ADCH << 8);
}

ISR(TIMER1_COMPA_vect)
{
    measure = 1;
}

ISR(TIMER0_COMPA_vect)
{
    PORTA = 0;
    TCCR0B = 0x00;
    measure = 0;
}

#define F_CPU 128000  // 128kHz

static inline uint32_t read_adc(unsigned char pin)
{
    TIMSK0 &= (0xff ^ (0x01 << OCIE0A));	// Disable timer interrupt

    ADMUXA = pin;							// ADC<pin> used as input
    ADCSRA = 0x88;							// ADC enabled, ADC interrupts enabled
    ADCSRA |= (0x01 << ADSC);				// Start ADC

    sleep_mode();							// Wait until ADC interrupt

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
    TIMSK0 = (0x01 << OCIE0A);  // Enable compare interrupt A
    TCCR0B = 0x00;				// Disable timer0
    OCR0A = 255;                // Compare value; 255/(128kHz/1024) = 2s

    TCCR1A = 0x00;
    TIMSK1 = (0x01 << OCIE1A);  // Enable compare interrupt A
    TCCR1B = ((0x01 << WGM12) | 0x05);		// Enable timer1, clock prescaler 1024
    OCR1AH = (1250 >> 8);		// 1250/(128kHz/1024) = 10s
    OCR1AL = (1250 & (0xff));

    ADMUXB = 0x00;		// Vcc as reference voltage for ADC, gain=1

    sei();
}

int main(void)
{
    setup_system();
    usart_init(1200);

    while (1)
    {
        sleep_mode();

        if (measure) {
            PORTA = (1 << 7);
            uint32_t analog = read_adc(0);
            itoa(analog, adc_str, 10);
            usart_println(adc_str, strlen(adc_str));

            if (analog < threshold) {
                PORTA = (1 << 3);
                TCNT0 = 0;
                TCCR0B = 0x05;     // Start timer0; clock prescaler 1024
            }
            else {
                PORTA = 0x00;
            }
        }
    }
}

