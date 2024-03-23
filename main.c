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
volatile uint8_t clock_state = 1; // 1 = "wach" (aktiver Betrieb), 0 = "schlafend" (Sleep-Mode)
volatile uint8_t pwm_value = 0xFF;
volatile uint8_t seconds = 0;

// Funktionsprototypen
void init_clock();

void update_time();

void display_time();

void setup_pwm_for_brightness();

void setup_timer2_asynchronous();

void setup_wakeup_interrupts();

void toggle_sleep_mode();

void startup_sequence();

void allLedsOn();

void allLedsOff();

uint8_t debounce_button_b(uint8_t button);

uint8_t debounce_button_d(uint8_t button);

ISR(TIMER2_OVF_vect) {
        seconds++;
        if (seconds >= 60) {
            seconds = 0;
            update_time();
        }
}

ISR(PCINT0_vect) {
        // Interrupt Service Routine für Pin-Change-Interrupt
        // Notwendig, um aus dem Sleep-Modus aufzuwachen, wird aber im Code nicht direkt verwendet
}

int main() {
    init_clock(); //Uhr initialisieren
    setup_timer2_asynchronous(); //Timer aktivieren
    sei(); // Global Interrupts aktivieren
    startup_sequence();
    display_time(); //Startzeit darstellen

    while (1) {
        if (!clock_state) {
            set_sleep_mode(SLEEP_MODE_ADC);
            sleep_mode();
        }
        if (debounce_button_b(BUTTON1)) {
            _delay_ms(50);
            toggle_sleep_mode();
        }
        if (debounce_button_b(BUTTON2)) {
            currentTime.minutes = (currentTime.minutes + 1) % 60;
            if (clock_state) {
                display_time();
            }
        }
        if (debounce_button_d(BUTTON3)) {
            currentTime.hours = (currentTime.hours + 1) % 24;
            if (clock_state) {
                display_time();
            }
        }
    }
}

void init_clock(void) {
    HOUR_LEDS_DDR |= 0xF8; // Stunden-LEDs als Ausgang - 11111000
    MINUTE_LEDS_DDR |= 0x3F; // Minuten-LEDs als Ausgang

    BUTTON_DDR_B &= ~((1 << PB0) | (1 << PB1)); // Taster an PB0 und PB1 als Eingang
    BUTTON_PORT_B |= (BUTTON1 | BUTTON2); // Pull-up Widerstände der Taster aktivieren

    BUTTON_DDR_D &= ~(1 << PD2); // Taster an PD2 als Eingang
    BUTTON_PORT_D |= BUTTON3; // Pull-up Widerstand des Tasters aktivieren

    PCICR |= (1 << PCIE0); // Pin-Change-Interrupts aktivieren (Notwendig für das Aufwachen)
    PCMSK0 |= (1 << PCINT0) | (1 << PCINT1); // Pin-Change-Interrupts für PB0 und PB1 aktivieren
}

void update_time() {
    currentTime.minutes++;
    if (currentTime.minutes >= 60) {
        currentTime.minutes = 0;
        currentTime.hours = (currentTime.hours + 1) % 24;
    }
    if (clock_state) {
        display_time();
    }
}

void display_time() {
    // Alle LEDs deaktivieren
    HOUR_LEDS_PORT &= ~0xF8; // Loesche die Bits von PD3 bis PD7
    MINUTE_LEDS_PORT &= ~0x3F; // Loesche die Bits von PC0 bis PC5

    // Setzen der Stunden-LEDs
    for (uint8_t i = 0; i < 5; i++) { // Durch die Fuenf
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
    //ToDo
}

// Initialisierungsfunktion fuer Timer2 im asynchronen Modus fuer
void setup_timer2_asynchronous() {
    ASSR |= (1 << AS2); // Aktiviere den asynchronen Modus von Timer2
    // Konfiguriere Timer2
    TCCR2A = 0; // Normaler Modus
    TCCR2B = (1 << CS22) | (1 << CS20); // Prescaler auf 128 setzen
    // Warte, bis die Update-Busy-Flags geloescht sind
    while (ASSR & ((1 << TCN2UB) | (1 << OCR2AUB) | (1 << OCR2BUB) | (1 << TCR2AUB) | (1 << TCR2BUB)));
    TIMSK2 = (1 << TOIE2); // Timer/Counter2 Overflow Interrupt Enable
}

// Funktion für Buttons an PB0 & PB1
uint8_t debounce_button_b(uint8_t button) {
    _delay_ms(50);
    return !(BUTTON_PIN_B & button);
}

// Funktion für Button an PD2
uint8_t debounce_button_d(uint8_t button) {
    _delay_ms(50);
    return !(BUTTON_PIN_D & button);
}

void toggle_sleep_mode(void) {
    if (clock_state) {
        // LEDs ausschalten
        HOUR_LEDS_PORT &= ~(0xF8);
        MINUTE_LEDS_PORT &= ~(0x3F);
        clock_state = 0; // Zustand der Uhr auf "schlafend" setzen
    } else {
        // LEDs entsprechend der aktuellen Uhrzeit wieder einschalten
        display_time();
        clock_state = 1; // Zustand auf "wach" setzen
    }
}

void startup_sequence() {
    //D7
    PORTD |= (1 << 7);
    _delay_ms(100);
    PORTD &= ~(1 << 7);

    //D6
    PORTD |= (1 << 6);
    _delay_ms(100);
    PORTD &= ~(1 << 6);

    //D5
    PORTD |= (1 << 5);
    _delay_ms(100);
    PORTD &= ~(1 << 5);

    //D4
    PORTD |= (1 << 4);
    _delay_ms(100);
    PORTD &= ~(1 << 4);

    //D3
    PORTD |= (1 << 3);
    _delay_ms(100);
    PORTD &= ~(1 << 3);

    //C5
    PORTC |= (1 << 5);
    _delay_ms(100);
    PORTC &= ~(1 << 5);

    //C4
    PORTC |= (1 << 4);
    _delay_ms(100);
    PORTC &= ~(1 << 4);

    //C3
    PORTC |= (1 << 3);
    _delay_ms(100);
    PORTC &= ~(1 << 3);

    //C2
    PORTC |= (1 << 2);
    _delay_ms(100);
    PORTC &= ~(1 << 2);

    //C1
    PORTC |= (1 << 1);
    _delay_ms(100);
    PORTC &= ~(1 << 1);

    //C0
    PORTC |= (1 << 0);
    _delay_ms(100);
    PORTC &= ~(1 << 0);

    _delay_ms(500);
    allLedsOn();
    _delay_ms(500);
    allLedsOff();
    _delay_ms(500);
}

void allLedsOn() {
    PORTC |= 0x3F; //00111111 - Minuten-LEDs
    PORTD |= 0xF8; //11111000 - Stunden-LEDs
}

void allLedsOff() {
    PORTC &= ~0x3F; //00111111 - Minuten-LEDs
    PORTD &= ~0xF8; //11111000 - Stunden-LEDs
}