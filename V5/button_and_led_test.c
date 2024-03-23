#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>

int debounce(int);
void turn_off_all_leds();

int main() {
    DDRC |= 0x3F; // 00111111 - Setzen der Minuten-LEDs als Ausgang
    DDRD |= 0xF8; // 11111000 - Setzten der Stunden-LEDs als Ausgang

    DDRB &= ~((1 << PB0) | (1 << PB1)); // PB0 + PB1 (Button 1 + Button 2) als Eingang definieren
    PORTB |= (1 << PB0) | (1 << PB1);   // PB0 + PB1 (Button 1 + Button 2) als Pull-Up konfigurieren
    DDRD &= ~(1 << PD2);                // PD2 (Button 3) als Eingang definieren
    PORTD |= (1 << PD2);                // PD2 (Button 3) als Pull-Up konfigurieren

    while (1) {
        if (debounce(0)) {
            PORTC |= 0x3F; // Minuten LEDs anschalten
        } else if (debounce(1)) {
            PORTD |= 0xF8; // Stunden LEDs anschalten
        } else if (debounce(2)) {
            turn_off_all_leds();
        }
    }
}

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

void turn_off_all_leds() {
    PORTC &= 0x00;
    PORTD &= 0x00;
}
