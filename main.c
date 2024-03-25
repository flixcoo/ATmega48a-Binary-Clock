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

// == Variablendefinition ==
// Zeitstruktur
typedef struct {
    uint8_t hours;
    uint8_t minutes;
} time_t;

// Systemvariablen
volatile time_t currentTime = {12, 0};
volatile uint8_t clock_state = 1; // 1 = "wach" (aktiver Betrieb), 0 = "schlafend" (Sleep-Mode)
volatile uint8_t seconds = 0;

// PWM-Variablem
volatile uint8_t max_dimming_steps = 10; // Anzahl der Dimmstufen
volatile uint8_t max_pwm_steps = 14; // Phasenlaenger der Pulsweitenmodulation
volatile uint8_t current_dimming_step = 0; // Aktuelle Dimmstufe
volatile uint8_t current_pwm_step = 0; // Aktueller Schritt der Pulsweitenmodulation

volatile uint8_t pwm_active = 1;
volatile uint8_t accuracy_test = 0;

// == Funktionsprototypen ==
// Systemkonfigurationen
void setup_timer1_for_pwm();
void setup_timer2_asynchronous();
void init_clock();

// Uhrenbetrieb
void update_time();
void display_time();
void toggle_sleep_mode();
void cycle_dimming_steps();

// Hilfsfunktionen
void startup_sequence();
uint8_t debounce_button_b(uint8_t button);
uint8_t debounce_button_d(uint8_t button);
void all_leds_on();
void all_leds_off();
void toggle_accuracy_test();

// == Interrupt Service Routinen ==
// Timer1 Compare
ISR(TIMER1_COMPA_vect) {
    if(pwm_active){
        current_pwm_step++;                         // Erhoehen des Phasen-Steps
        if (current_pwm_step >= max_pwm_steps) {    // Wenn max_pwm_steps ueberschritten wird
            current_pwm_step = 0;               // ... wieder auf 0 setzten
        }

        uint8_t leds_on = (current_pwm_step <
                           (max_dimming_steps - current_dimming_step)); // Verhaeltnis Low zu High Pegel

        if (clock_state) {                   // Uhr ist "wach"
            if (leds_on) {                  // High-Pegel-Phase
                display_time();             // Setzte High Pegel
            } else {                        // Low-Pegel Phase
                all_leds_off();             // Setzte Low Pegel
            }
        }
    }
}

// Timer2 Overflow
ISR(TIMER2_OVF_vect) {
    if(accuracy_test){
        PORTD ^= (1 << PD0);
    }
    // Wechseln des Zustands von PD0
    seconds++;              // Bei Overflow: zaehle die Sekunden hoch
    if (seconds >= 60) {    // Wenn Sekunden ueber 60
        seconds = 0;        // Setzte Sekunden auf 0 zurueck
        update_time();      // Aktualisiere die interne Zeit
    }
}

ISR(PCINT0_vect) {
    // Interrupt Service Routine für Pin-Change-Interrupt
    // Notwendig, um aus dem Sleep-Modus aufzuwachen, keine weitere Funktion und deshalb auch kein Body
}

int main() {
    init_clock();                   //Uhr initialisieren
    setup_timer2_asynchronous();    //Timer2 im asynchronen Modus konfigurieren
    startup_sequence();             // LED-Start-Sequenz durchlaufen lassen
    display_time();                 // Initial die Zeit anzeigen
    setup_timer1_for_pwm();         // Timer1 fuer die PWM konfigurieren
    sei();                          // Globale Interrupts aktivieren

    while (1) {
        // Ist die Uhr im Energiesparmodus?
        if (!clock_state) {
            set_sleep_mode(SLEEP_MODE_ADC); // Konfiguriere Energiesparmodus
            sleep_mode();                   // Aktiviere Energiesparmodus
        }

        if((debounce_button_d(BUTTON1)) && (debounce_button_b(BUTTON2))){ // Taster 1 + 2 werden gedrueckt
            toggle_accuracy_test();
            _delay_ms(100); // Entprellung hoeher, da zwei Taster gedrueckt werden
        }
        if ((debounce_button_d(BUTTON3)) && (debounce_button_b(BUTTON2))) { // Taster 2 + 3 werden gedrueckt
            cycle_dimming_steps();
            _delay_ms(100); // Entprellung hoeher, da zwei Taster gedrueckt werden
        } else if (debounce_button_b(BUTTON2) && !accuracy_test) {
            currentTime.minutes = (currentTime.minutes + 1) % 60; // Erhoehe Minuten um 1, maximal bis 60
            _delay_ms(50); // Entprellung
        } else if (debounce_button_d(BUTTON3) && !accuracy_test) {
            currentTime.hours = (currentTime.hours + 1) % 24; // Erhoehe Stunden um 1, maximal bis 24
            _delay_ms(50);  // Entprellung
        } else if (debounce_button_b(BUTTON1) && !accuracy_test) {
            toggle_sleep_mode();            // Schalte den Energiesparmodus um
            _delay_ms(50);                 // Entprellung
        }
    }
}

// Timer 2 im asynchronen Modus fuer Sekundezaehlung konfigurieren
void setup_timer2_asynchronous() {
    ASSR |= (1 << AS2);                 // Aktiviere den asynchronen Modus von Timer2
    TCCR2A = 0;                         // Normaler Modus des Timer2
    TCCR2B = (1 << CS22) | (1 << CS20); // Prescaler auf 128 setzen
    // Warte, bis die Update-Busy-Flags geloescht sind
    while (ASSR & ((1 << TCN2UB) | (1 << OCR2AUB) | (1 << OCR2BUB) | (1 << TCR2AUB) | (1 << TCR2BUB)));
    TIMSK2 = (1 << TOIE2);              // Aktiviere den Timer2 Overflow Interrupt
}

// Timer1 für Pulsweitenmodulation konfigurieren
void setup_timer1_for_pwm() {
    TCCR1B |= (1 << WGM12);                 // CTC Modus
    OCR1A = 15;                             // 10 ms bei 8 MHz und Prescaler von 64
    TIMSK1 |= (1 << OCIE1A);                // Timer1 Compare Match Interrupt aktivieren
    TCCR1B |= (1 << CS11) | (1 << CS10);    // Prescaler auf 64 setzen
}

// Register und Ports konfigurieren
void init_clock(void) {
    HOUR_LEDS_DDR |= 0xF8;                      // Stunden-LEDs als Ausgang - 11111000
    MINUTE_LEDS_DDR |= 0x3F;                    // Minuten-LEDs als Ausgang - 00111111

    BUTTON_DDR_B &= ~((1 << PB0) | (1 << PB1)); // Taster an PB0 und PB1 als Eingang
    BUTTON_PORT_B |= (BUTTON1 | BUTTON2);       // Pull-up Widerstände der Taster aktivieren

    BUTTON_DDR_D &= ~(1 << PD2);                // Taster an PD2 als Eingang
    BUTTON_PORT_D |= BUTTON3;                   // Pull-up Widerstand des Tasters aktivieren

    PCICR |= (1 << PCIE0);                      // Pin-Change-Interrupts aktivieren (Notwendig für das Aufwachen)
    PCMSK0 |= (1 << PCINT0);                    // Pin-Change-Interrupts für PB0

    DDRD |= (1<<0);                             // Setze PD0 als Ausgang fuer die Zeitmessung
}

// Interne Zeit aktualisieren
void update_time() {
    currentTime.minutes++;                                  // Erhoehe Minuten um 1
    if (currentTime.minutes >= 60) {                        // Wenn Minuten 60 erreicht haben
        currentTime.minutes = 0;                            // Minuten zurueck auf 0 setzten
        currentTime.hours = (currentTime.hours + 1) % 24;   // ... und dafuer Stunden eine hoch setzten
    }
}

// Zeit auf den LEDs anzeigen
void display_time() {
    all_leds_off();                 // Aktuell angezeigte Zeit zuruecksetzten

    // Setzen der Stunden-LEDs
    for (uint8_t i = 0; i < 5; i++) {           // Durch die Fuenf Stunden-LEDs iterieren
        if (currentTime.hours & (1 << i)) {     // Die Bits der aktuellen Zeit werden der Reihe nach ausgelesen
            HOUR_LEDS_PORT |= (1 << (PD7 - i)); // ... und auf die Stunden LEDs geschrieben
        }
    }

    // Setzen der Minuten-LEDs in binaerer Form
    for (uint8_t i = 0; i < 6; i++) {           // Durch die Sechs Minuten-LEDs iterieren
        if (currentTime.minutes & (1 << i)) {   // Die Bits der aktuellen Zeit werden der Reihe nach ausgelesen
            MINUTE_LEDS_PORT |= (1 << 5 - i);   // ... und auf die Stunden LEDs geschrieben
        }
    }
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

// Stromsparmodus de-/aktivieren
void toggle_sleep_mode() {
    if (clock_state) {      // Uhr ist im Zustand "wach"
        all_leds_off();     // Alle LEDs deaktivieren
        clock_state = 0;    // Zustand der Uhr auf "schlafend" setzen
    } else {                // Uhr ist im Zustand "schlafend"
        display_time();     // LEDs entsprechend der aktuellen Uhrzeit wieder einschalten
        clock_state = 1;    // Zustand auf "wach" setzen
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
    _delay_ms(200);
}

// Alle LEDs aktivieren
void all_leds_on() {
    PORTC |= 0x3F; //00111111 - Minuten-LEDs
    PORTD |= 0xF8; //11111000 - Stunden-LEDs
}

// Alle LEDs deaktivieren
void all_leds_off() {
    PORTC &= ~0x3F; //00111111 - Minuten-LEDs
    PORTD &= ~0xF8; //11111000 - Stunden-LEDs
}

void toggle_accuracy_test(){
    if(accuracy_test){
        all_leds_on();
        _delay_ms(300);
        all_leds_off();
        _delay_ms(300);
        all_leds_on();
        _delay_ms(300);
        all_leds_off();
        _delay_ms(300);
        all_leds_on();
        _delay_ms(300);
        all_leds_off();

        accuracy_test = 0;
        pwm_active = 1;
    } else{
        accuracy_test = 1;
        pwm_active = 0;

        all_leds_on();
        _delay_ms(2000);
        all_leds_off();
    }
}

void cycle_dimming_steps(){
    if (current_dimming_step > 0) { // Solange die aktuelle Stufe groesser als 0 ist
        current_dimming_step--; // ... wird eine Stufe abgezogen
    } else {
        current_dimming_step = max_dimming_steps - 1; // Wenn bei 0 angekommen, zuruecksetzten auf Stufe 10 (bzw. 9)
    }
}