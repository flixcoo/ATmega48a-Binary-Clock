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

volatile uint8_t dimming_enabled = 0; // Dimm-Funktion ist standardmäßig deaktiviert
volatile uint8_t dimming_counter = 0;
volatile uint8_t dimming_phase = 0; // 0: LEDs aus, 1: Dimmen aktiv
volatile uint8_t dimming_step = 0; // Schrittweite für das Dimmen (0-3 für 25% Schritte)


// Funktionsprototypen
void init_clock();

void update_time();

void display_time();

void setup_pwm_for_brightness();

void setup_timer2_asynchronous();

void setup_wakeup_interrupts();

void toggle_sleep_mode();

void startup_sequence();

void all_leds_on();

void all_leds_off();

void toggle_dimming();

uint8_t debounce_button_b(uint8_t button);

uint8_t debounce_button_d(uint8_t button);

ISR(TIMER1_COMPA_vect) {
        // Erhöht den Dimming-Zähler in jedem Timer-Intervall
        dimming_counter++;
        if (dimming_counter >= 4) { // Reset nach 40*10ms = 400ms für einen vollständigen Zyklus
            dimming_counter = 0;
        }

        uint8_t leds_on = (dimming_counter < (1 * (4 - dimming_step))); // Anpassung für korrektes Dimmverhalten

        if (leds_on) {
            // LEDs entsprechend der aktuellen Uhrzeit wieder einschalten
            display_time();
        } else {
            // LEDs ausschalten
            HOUR_LEDS_PORT &= ~0xF8;
            MINUTE_LEDS_PORT &= ~0x3F;
        }
}


ISR(TIMER2_OVF_vect) {
        seconds++;
        if (seconds >= 60) {
            seconds = 0;
            update_time();
        }
}

ISR(PCINT0_vect) {
        // Interrupt Service Routine für Pin-Change-Interrupt
        // Notwendig, um aus dem Sleep-Modus aufzuwachen, keine weitere Funktion und deshalb auch kein Body
}

int main() {
    init_clock(); //Uhr initialisieren
    setup_timer2_asynchronous(); //Timer aktivieren
    sei(); // Global Interrupts aktivieren
    startup_sequence();
    display_time(); //Startzeit darstellen

    // Timer1 für Software-PWM konfigurieren
    TCCR1B |= (1 << WGM12); // CTC Modus
    OCR1A = 78; // 10 ms bei 8 MHz und Prescaler von 64
    TIMSK1 |= (1 << OCIE1A); // Timer1 Compare Match Interrupt aktivieren
    TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler auf 64 setzen

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
            dimming_step = (dimming_step + 1) % 4; // Durchlaufe die Dimmstufen
            _delay_ms(50); // Entprellung
        }

        if ((debounce_button_d(BUTTON3)) && (debounce_button_b(BUTTON2))){
            //ToDo
        }
    }
}

void init_clock(void) {
    HOUR_LEDS_DDR |= 0xF8; // Stunden-LEDs als Ausgang - 11111000
    MINUTE_LEDS_DDR |= 0x3F; // Minuten-LEDs als Ausgang - 00111111

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

void all_leds_on() {
    PORTC |= 0x3F; //00111111 - Minuten-LEDs
    PORTD |= 0xF8; //11111000 - Stunden-LEDs
}

void all_leds_off() {
    PORTC &= ~0x3F; //00111111 - Minuten-LEDs
    PORTD &= ~0xF8; //11111000 - Stunden-LEDs
}