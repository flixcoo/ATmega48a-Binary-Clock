#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>

void allLedsOn();
void allLedsOff();

void main() {
    DDRC = (1 << 5);
    uint8_t time = 100;
    while (1) {

        //C 0 - 5
        //D 3 - 7
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
        allLedsOn();
        _delay_ms(500);
        allLedsOff();
        _delay_ms(500);


    }
}

void allLedsOn() {
    PORTC |= (1 << 0);
    PORTC |= (1 << 1);
    PORTC |= (1 << 2);
    PORTC |= (1 << 3);
    PORTC |= (1 << 4);
    PORTC |= (1 << 5);
    PORTD |= (1 << 3);
    PORTD |= (1 << 4);
    PORTD |= (1 << 5);
    PORTD |= (1 << 6);
    PORTD |= (1 << 7);
}

void allLedsOff() {
    PORTC &= ~(1 << 0);
    PORTC &= ~(1 << 1);
    PORTC &= ~(1 << 2);
    PORTC &= ~(1 << 3);
    PORTC &= ~(1 << 4);
    PORTC &= ~(1 << 5);
    PORTD &= ~(1 << 3);
    PORTD &= ~(1 << 4);
    PORTD &= ~(1 << 5);
    PORTD &= ~(1 << 6);
    PORTD &= ~(1 << 7);
}