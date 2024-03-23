#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>

int debounce(int button) {
    static uint8_t buttonState[3] = {0}; 
    static uint8_t buttonPress[3] = {0}; 

    uint8_t portBit;
    if (button < 2) {
        portBit = button; // PB0, PB1 for button 0, 1
    } else {
        portBit = PD2; // PD2 for button 2
    }

    uint8_t isButtonPressed = (button < 2) ? !(PINB & (1 << portBit)) : !(PIND & (1 << portBit));
    
    if (isButtonPressed && (buttonState[button] == 0)) {
        _delay_ms(10); 
        isButtonPressed = (button < 2) ? !(PINB & (1 << portBit)) : !(PIND & (1 << portBit));
        if (isButtonPressed) {
            buttonPress[button] = 1;
            buttonState[button] = 1;
        }
    } else if (!isButtonPressed && (buttonState[button] == 1)) {
        buttonPress[button] = 0;
        buttonState[button] = 0;
    }

    return buttonPress[button];
}

void turnOffAllLEDs() {
    PORTC = 0x00;
    PORTD &= 0x07; // Clears only the upper 5 bits (PD3 to PD7)
}

int main(void) {
    // Configure PC0-PC5 and PD3-PD7 as output for LEDs
    DDRC |= 0x3F; // 0011 1111 - Sets PC0 to PC5 as output
    DDRD |= 0xF8; // 1111 1000 - Sets PD3 to PD7 as output

    // Configure PB0, PB1, and PD2 as input with pull-up
    DDRB &= ~((1 << PB0) | (1 << PB1));
    PORTB |= (1 << PB0) | (1 << PB1);
    DDRD &= ~(1 << PD2);
    PORTD |= (1 << PD2);

    while (1) {
        if (debounce(0)) {
            PORTC = 0x3F; // Turn on all PC LEDs
            PORTD |= 0xF8; // Turn on PD3 to PD7 LEDs
        } else if (debounce(1)) {
            PORTC = 0x00; // Turn off PC LEDs
            PORTD |= 0xF8; // Turn on PD3 to PD7 LEDs
        } else if (debounce(2)) {
            turnOffAllLEDs();
        }
    }

    return 0;
}
