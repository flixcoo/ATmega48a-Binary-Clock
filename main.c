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
#define BUTTON_DDR   DDRD
#define BUTTON_PORT  PORTD
#define BUTTON_PIN   PIND
#define BUTTON1      _BV(PD0)
#define BUTTON2      _BV(PD1)
#define BUTTON3      _BV(PD2)

// Zeitstruktur
typedef struct {
    uint8_t hours;
    uint8_t minutes;
} time_t;

volatile time_t currentTime = {12, 0}; // Initialzeit setzen
volatile uint8_t clock_state = 1; // Uhr eingeschaltet starten
volatile uint8_t pwm_value = 0xFF; // Initialer PWM-Helligkeitswert

// Funktionsprototypen
void init_clock(void);
void update_time(int direction);
void display_time(void);
void setup_pwm_for_brightness(void);
void setup_timer2_asynchronous(void);
void sleep_mode_activate(void);
uint8_t debounce_button(uint8_t button);

// Timer2 Overflow Interrupt Service Routine
ISR(TIMER2_OVF_vect) {
    // Diese ISR wird aufgerufen, wenn Timer2 überläuft (einmal pro Sekunde bei 32.768 kHz)
    if(clock_state) {
        update_time(1);
    }
}

// Timer0 Compare Match A Interrupt Service Routine für PWM-Helligkeitssteuerung
ISR(TIMER0_COMPA_vect) {
    // Helligkeitssteuerung hier nicht direkt nötig, da PWM durch Timer0 Hardware gesteuert wird
}

int main(void) {
    init_clock();
    setup_pwm_for_brightness();
    setup_timer2_asynchronous();
    sei(); // Global Interrupts aktivieren

    while (1) {
        if (debounce_button(BUTTON1)) {
            clock_state ^= 1; // Zustand umschalten
            if(!clock_state) {
                // LEDs ausschalten wenn die Uhr ausgeschaltet ist
                HOUR_LEDS_PORT &= ~((1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7));
                MINUTE_LEDS_PORT &= ~((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5));
            }
        }

        if(debounce_button(BUTTON2)) {
            sleep_mode_activate();
        }

        if(clock_state) {
            display_time(); // Zeit anzeigen wenn die Uhr eingeschaltet ist
        }
    }
}

void init_clock(void) {
    HOUR_LEDS_DDR |= 0xF8; // Stunden-LEDs als Ausgang
    MINUTE_LEDS_DDR |= 0x3F; // Minuten-LEDs als Ausgang
    BUTTON_DDR &= ~(BUTTON1 | BUTTON2); // Taster als Eingang
    BUTTON_PORT |= (BUTTON1 | BUTTON2 | BUTTON3); // Aktiviere Pull-up für Taster 1, 2 und 3
}

void update_time(int direction) {
    if(direction > 0) { // Hochzählen
        currentTime.minutes++;
        if(currentTime.minutes >= 60) {
            currentTime.minutes = 0;
            currentTime.hours = (currentTime.hours + 1) % 24;
        }
    } else { // Runterzählen
        if(currentTime.minutes == 0) {
            currentTime.minutes = 59;
            currentTime.hours = (currentTime.hours == 0) ? 23 : currentTime.hours - 1;
        } else {
            currentTime.minutes--;
        }
    }
    display_time(); // Aktualisiere die Anzeige nach jeder Zeitänderung
}

void display_time(void) {
    // Lösche zuerst die aktuellen LED-Anzeigen für Stunden und Minuten
    HOUR_LEDS_PORT &= ~((1 << PD3) | (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7));
    MINUTE_LEDS_PORT &= ~0x3F; // Alle Minuten-LEDs löschen

    // Berechnung der Stunden-LEDs
    // Das Binärmuster der Stunden auf den LEDs PD3 bis PD7 darstellen
    for (uint8_t i = 0; i < 5; i++) {
        if (currentTime.hours & (1 << i)) {
            HOUR_LEDS_PORT |= (1 << (i + PD3));
        }
    }

    // Berechnung der Minuten-LEDs in einer echten Binärdarstellung
    // Das Binärmuster der Minuten auf den LEDs PC0 bis PC5 darstellen
    for (uint8_t i = 0; i < 6; i++) {
        if (currentTime.minutes & (1 << i)) {
            MINUTE_LEDS_PORT |= (1 << i);
        }
    }
}

void setup_pwm_for_brightness(void) {
    TCCR0A |= (1 << WGM01) | (1 << WGM00); // Fast-PWM-Modus
    TCCR0A |= (1 << COM0A1); // Nicht-invertierender Modus
    TCCR0B |= (1 << CS00); // Kein Prescaler
    OCR0A = pwm_value; // Initialer PWM-Wert
}

// Initialisierungsfunktion für Timer2 im asynchronen Modus
void setup_timer2_asynchronous(void) {
    ASSR |= (1<<AS2); // Aktiviere den asynchronen Modus von Timer2
    // Konfiguriere Timer2
    TCCR2A = 0; // Normaler Modus
    TCCR2B = (1<<CS22) | (1<<CS20); // Prescaler auf 128 setzen
    // Warte, bis die Update-Busy-Flags gelöscht sind
    while (ASSR & ((1<<TCN2UB)|(1<<OCR2AUB)|(1<<OCR2BUB)|(1<<TCR2AUB)|(1<<TCR2BUB)));
    TIMSK2 = (1<<TOIE2); // Timer/Counter2 Overflow Interrupt Enable
}

void sleep_mode_activate(void) {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
}

uint8_t debounce_button(uint8_t button) {
    _delay_ms(50); // Kurze Verzögerung für Entprellung
    return !(BUTTON_PIN & button);
}
