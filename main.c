#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define F_CPU 32768UL

// LED-Konfigurationen
#define HOUR_LEDS_DDR    DDRD
#define HOUR_LEDS_PORT   PORTD
#define MINUTE_LEDS_DDR  DDRC
#define MINUTE_LEDS_PORT PORTC

// Taster-Konfigurationen
#define BUTTON_DDR_B DDRB
#define BUTTON_PORT_B PORTB
#define BUTTON_PIN_B PINB
#define BUTTON1 _BV(PB0) // Taster 1 an PB0
#define BUTTON2 _BV(PB1) // Taster 2 an PB1

#define BUTTON_DDR_D DDRD
#define BUTTON_PORT_D PORTD
#define BUTTON_PIN_D PIND
#define BUTTON3 _BV(PD2) // Taster 3 an PD2

// Zeitstruktur
typedef struct {
    uint8_t hours;
    uint8_t minutes;
} time_t;

volatile time_t currentTime = {12, 0};
volatile uint8_t clock_state = 1;
volatile uint8_t pwm_value = 0xFF;
volatile uint8_t seconds = 0;

// Funktionsprototypen
void init_clock();
void update_time();
void display_time();
void setup_pwm_for_brightness();
void setup_timer2_asynchronous();
void sleep_mode_activate();
uint8_t debounce_button_b(uint8_t button);
uint8_t debounce_button_d(uint8_t button);

ISR(TIMER2_OVF_vect) {
    if(clock_state) {
        seconds++;
        if(seconds >= 60) {
            seconds = 0;
            update_time();
        }
    }
}

// Timer0 Compare Match A Interrupt Service Routine fuer PWM-Helligkeitssteuerung
ISR(TIMER0_COMPA_vect) {
        // Helligkeitssteuerung hier nicht direkt noetig, da PWM durch Timer0 Hardware gesteuert wird
}


int main() {
    init_clock();
    //setup_pwm_for_brightness();
    setup_timer2_asynchronous();
    sei(); // Global Interrupts aktivieren
    display_time();
    while (1) {
        if (debounce_button_b(BUTTON1)) {
            HOUR_LEDS_PORT &= ~(0x00);
            MINUTE_LEDS_PORT &= ~(0x00);
        }

        if (debounce_button_b(BUTTON2)) {
            currentTime.minutes = (currentTime.minutes + 1) % 60;
            display_time();
        }

        if (debounce_button_d(BUTTON3)) {
            currentTime.hours = (currentTime.hours + 1) % 24;
            display_time();
        }
    }
}

void init_clock() {
    HOUR_LEDS_DDR |= 0xF8; // Stunden-LEDs als Ausgang - 11111000
    MINUTE_LEDS_DDR |= 0x3F; // Minuten-LEDs als Ausgang

    BUTTON_DDR_B &= ~((1 << PB0) | (1 << PB1)); // Taster als Eingang
    BUTTON_PORT_B |= (BUTTON1 | BUTTON2); // Pull-up Widerstaende aktivieren

    BUTTON_DDR_D &= ~(1 << PD2); // Taster als Eingang
    BUTTON_PORT_D |= BUTTON3; // Pull-up Widerstand aktivieren
}

void update_time() {
    currentTime.minutes++;
    if (currentTime.minutes >= 60) {
        currentTime.minutes = 0;
        currentTime.hours = (currentTime.hours + 1) % 24;
    }
    display_time();
}

void display_time() {
    // Loesche zuerst die aktuellen LED-Anzeigen fuer Stunden und Minuten
    HOUR_LEDS_PORT &= ~0xF8; // Loescht die Bits von PD3 bis PD7
    MINUTE_LEDS_PORT &= ~0x3F; // Loescht die Bits von PC0 bis PC5

    // Setzen der Stunden-LEDs in binaerer Form
    for (uint8_t i = 0; i < 5; i++) { // Angenommen, es gibt 5 LEDs fuer Stunden
        if (currentTime.hours & (1 << i)) {
            HOUR_LEDS_PORT |= (1 << (PD7 - i));
        }
    }

    // Setzen der Minuten-LEDs in binaerer Form
    for (uint8_t i = 0; i < 6; i++) { // Angenommen, es gibt 6 LEDs fuer Minuten
        if (currentTime.minutes & (1 << i)) {
            MINUTE_LEDS_PORT |= (1 << 5 - i);
        }
    }
}

void setup_pwm_for_brightness() {
    TCCR0A |= (1 << WGM01) | (1 << WGM00); // Fast-PWM-Modus
    TCCR0A |= (1 << COM0A1); // Nicht-invertierender Modus
    TCCR0B |= (1 << CS00); // Kein Prescaler
    OCR0A = pwm_value; // Initialer PWM-Wert
}

// Initialisierungsfunktion fuer Timer2 im asynchronen Modus fuer
void setup_timer2_asynchronous() {
    ASSR |= (1<<AS2); // Aktiviere den asynchronen Modus von Timer2
    // Konfiguriere Timer2
    TCCR2A = 0; // Normaler Modus
    TCCR2B = (1<<CS22) | (1<<CS20); // Prescaler auf 128 setzen
    // Warte, bis die Update-Busy-Flags geloescht sind
    while (ASSR & ((1<<TCN2UB)|(1<<OCR2AUB)|(1<<OCR2BUB)|(1<<TCR2AUB)|(1<<TCR2BUB)));
    TIMSK2 = (1<<TOIE2); // Timer/Counter2 Overflow Interrupt Enable
}

void sleep_mode_activate() {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
}

uint8_t debounce_button_b(uint8_t button) {
    _delay_ms(50);
    return !(BUTTON_PIN_B & button);
}

uint8_t debounce_button_d(uint8_t button) {
    _delay_ms(50);
    return !(BUTTON_PIN_D & button);
}