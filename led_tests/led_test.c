#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>

void all_leds_on();
void all_leds_off();

void main() {
    uint8_t time = 100;
    while (1) {
        DDRC = (0x3F);//00111111
        DDRD = (0xF8);//11111000

        for (int i = 7; i >= 3; i--) {
            PORTD |= (1 << i);
            _delay_ms(100);
            PORTD &= ~(1 << i);
        }

        for (int i = 5; i >= 0; i--) {
            PORTC |= (1 << i);
            _delay_ms(100);
            PORTC &= ~(1 << i);
        }

        _delay_ms(500);
        all_leds_on();
        _delay_ms(500);
        all_leds_off();
        _delay_ms(500);
    }
}

void all_leds_on() {
    PORTC |= 0x3F; //00111111 - Minuten-LEDs
    PORTD |= 0xF8; //11111000 - Stunden-LEDs
}

void all_leds_off() {
    PORTC &= ~0x3F; //00111111 - Minuten-LEDs
    PORTD &= ~0xF8; //11111000 - Stunden-LEDs
}