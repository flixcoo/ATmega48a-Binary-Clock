#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define F_CPU 32768UL // CPU frequency set to 32.768 kHz (RTC module frequency)

// LED configurations
#define HOUR_LEDS_DDR    DDRD
#define HOUR_LEDS_PORT   PORTD
#define HOUR_LEDS        0xF8
#define MINUTE_LEDS_DDR  DDRC
#define MINUTE_LEDS_PORT PORTC
#define MINUTE_LEDS      0x3F

// Button configurations
#define BUTTON1 _BV(PB0) // Button 1 at PB0
#define BUTTON2 _BV(PB1) // Button 2 at PB1
#define BUTTON3 _BV(PD2) // Button 3 at PD2

// == Variable definitions ==
// Variable to hold the current time
typedef struct {
    uint8_t hours;
    uint8_t minutes;
} time_t;

// System variables
volatile time_t currentTime = {12, 0};      // Initial time - 12:00
volatile uint8_t clock_state = 1;           // 1 = "awake" (active mode), 0 = "sleeping" (sleep mode)
volatile uint8_t seconds = 0;               // Current seconds counter

// PWM variables
volatile uint8_t max_dimming_steps = 4;     // Number of brightness steps
volatile uint8_t max_pwm_steps = 12;        // PWM phase length
volatile uint8_t current_dimming_step = 0;  // Current brightness step
volatile uint8_t current_pwm_step = 0;      // Current PWM step

// Accuracy measurement
volatile uint8_t pwm_active = 1;            // 1 = PWM active, 0 = PWM inactive
volatile uint8_t accuracy_test = 0;         // 1 = accuracy test mode active, 0 = inactive

// Auto-sleep
volatile uint8_t sec_sleep_count = 0;       // Seconds counter for auto-sleep
volatile uint8_t auto_sleep_limit = 150;    // Seconds until auto-sleep activates

// LED test
volatile uint8_t led_test = 0;

// == Function prototypes ==
// System configurations
void setup_timer1_for_pwm();
void setup_timer2_asynchronous();
void init_clock();

// Clock operations
void update_time();
void display_time();
void toggle_sleep_mode();
void cycle_dimming_steps();
void toggle_accuracy_test();
void toggle_led_test();

// Helper functions
void startup_sequence();
void sleep_mode_sequence();
void wakeup_sequence();
uint8_t debounce_button_b(uint8_t button);
uint8_t debounce_button_d(uint8_t button);
void all_leds_on();
void all_leds_off();

// == Interrupt Service Routines ==
// Timer1 Compare
ISR(TIMER1_COMPA_vect) {
    if(pwm_active){
        current_pwm_step++;                         // Increment phase step
        if (current_pwm_step >= max_pwm_steps) {    // If max steps reached
            current_pwm_step = 0;                   // Reset to 0
        }

        uint8_t leds_on = (current_pwm_step < (max_dimming_steps - current_dimming_step + 1)); // Low/High ratio

        if (leds_on) {          // High phase
            display_time();     // Set high level
        } else {                // Low phase
            all_leds_off();     // Set low level
        }
    }
}

// Timer2 Overflow
ISR(TIMER2_OVF_vect) {
    if(accuracy_test){
        PORTD ^= (1 << PD0);
    }
    if(clock_state){
        sec_sleep_count++;
    }
    // Toggle PD0 state
    seconds++;              // On overflow: increment seconds
    if (seconds >= 60) {    // If seconds reach 60
        seconds = 0;        // Reset seconds
        update_time();      // Update internal time
    }
}

ISR(PCINT0_vect) {
    // Pin-Change Interrupt Service Routine
    // Required to wake from sleep mode, no additional functionality needed
}

int main() {
    init_clock();                   // Initialize clock
    setup_timer2_asynchronous();    // Configure Timer2 in asynchronous mode
    startup_sequence();             // Run LED startup sequence
    display_time();                 // Display initial time
    setup_timer1_for_pwm();         // Configure Timer1 for PWM
    sei();                          // Enable global interrupts

    while (1) {
        if (!clock_state) {                         // Is clock in sleep mode?
            set_sleep_mode(SLEEP_MODE_PWR_SAVE);    // Configure power save mode
            sleep_mode();                           // Activate sleep mode
        }
        if(sec_sleep_count >= auto_sleep_limit){
            sec_sleep_count = 0;
            toggle_sleep_mode();
        }

        if ((debounce_button_b(BUTTON1)) && (debounce_button_b(BUTTON2))) { // Button 1 + 2 pressed
            sec_sleep_count = 0;
            toggle_accuracy_test();
            _delay_ms(100); // Debounce
        }
        if(!accuracy_test)
        {
            if ((debounce_button_b(BUTTON2)) && (debounce_button_d(BUTTON3))) { // Button 2 + 3 pressed
                sec_sleep_count = 0;    // Reset auto-sleep timer
                cycle_dimming_steps();  // Next brightness step
                _delay_ms(100);         // Debounce
            } else if ((debounce_button_b(BUTTON1)) && (debounce_button_d(BUTTON3))) { // Button 1 + 3 pressed
                sec_sleep_count = 0;    // Reset auto-sleep timer
                toggle_led_test();
            } else if (debounce_button_b(BUTTON1)) {
                toggle_sleep_mode();    // Toggle sleep mode
                _delay_ms(50);          // Debounce
            } else if (debounce_button_b(BUTTON2)) {
                sec_sleep_count = 0;
                currentTime.minutes = (currentTime.minutes + 1) % 60;   // Increment minutes (0-59)
                _delay_ms(50);                                          // Debounce
            } else if (debounce_button_d(BUTTON3)) {
                sec_sleep_count = 0;
                currentTime.hours = (currentTime.hours + 1) % 24;   // Increment hours (0-23)
                _delay_ms(50);                                      // Debounce
            }
        }
    }
}

// Configure Timer2 in asynchronous mode for seconds counting
void setup_timer2_asynchronous() {
    ASSR |= (1 << AS2);                 // Enable Timer2 asynchronous mode
    TCCR2A = 0;                         // Timer2 normal mode
    TCCR2B = (1 << CS22) | (1 << CS20); // Set prescaler to 128
    // Wait until update busy flags are cleared
    while (ASSR & ((1 << TCN2UB) | (1 << OCR2AUB) | (1 << OCR2BUB) | (1 << TCR2AUB) | (1 << TCR2BUB)));
    TIMSK2 = (1 << TOIE2);              // Enable Timer2 overflow interrupt
}

// Configure Timer1 for PWM
void setup_timer1_for_pwm() {
    TCCR1B |= (1 << WGM12);                 // CTC mode
    OCR1A = 15;
    TIMSK1 |= (1 << OCIE1A);                // Enable Timer1 compare match interrupt
    TCCR1B |= (1 << CS11) | (1 << CS10);    // Set prescaler to 64
}

// Configure registers and ports
void init_clock() {
    HOUR_LEDS_DDR |= HOUR_LEDS;                 // Hour LEDs as output - 11111000
    MINUTE_LEDS_DDR |= MINUTE_LEDS;             // Minute LEDs as output - 00111111

    DDRB &= ~((1 << PB0) | (1 << PB1));         // Buttons at PB0 and PB1 as input
    PORTB |= (BUTTON1 | BUTTON2);               // Enable button pull-up resistors

    DDRD &= ~(1 << PD2);                        // Button at PD2 as input
    PORTD |= BUTTON3;                           // Enable button pull-up resistor

    PCICR |= (1 << PCIE0);                      // Enable pin-change interrupts (needed for wake-up)
    PCMSK0 |= (1 << PCINT0);                    // Pin-change interrupt for PB0

    DDRD |= (1<<0);                             // Set PD0 as output for timing measurement
}

// Update internal time
void update_time() {
    currentTime.minutes++;                                  // Increment minutes
    if (currentTime.minutes >= 60) {                        // If minutes reach 60
        currentTime.minutes = 0;                            // Reset minutes
        currentTime.hours = (currentTime.hours + 1) % 24;   // Increment hours
    }
}

// Display time on LEDs
void display_time() {
    all_leds_off();                             // Clear current display

    // Set hour LEDs
    for (uint8_t i = 0; i < 5; i++) {           // Iterate through 5 hour LEDs
        if (currentTime.hours & (1 << i)) {     // Read current time bits
            HOUR_LEDS_PORT |= (1 << (PD7 - i)); // Write to hour LEDs
        }
    }

    // Set minute LEDs
    for (uint8_t i = 0; i < 6; i++) {           // Iterate through 6 minute LEDs
        if (currentTime.minutes & (1 << i)) {   // Read current time bits
            MINUTE_LEDS_PORT |= (1 << 5 - i);   // Write to minute LEDs
        }
    }
}

void cycle_dimming_steps(){
    if (current_dimming_step > 0) { // While current step > 0
        current_dimming_step--; // Decrement step
    } else {
        current_dimming_step = max_dimming_steps; // If at 0, reset to max step
    }
}

// Toggle sleep mode
void toggle_sleep_mode() {
    if (clock_state) {      // Clock is "awake"
        all_leds_off();     // Turn off all LEDs
        pwm_active = 0;     // Disable PWM
        sleep_mode_sequence();
        clock_state = 0;    // Set state to "sleeping"
    } else {                // Clock is "sleeping"
        wakeup_sequence();
        display_time();     // Turn on LEDs according to current time
        clock_state = 1;    // Set state to "awake"
        pwm_active = 1;     // Enable PWM
    }
}

// Toggle accuracy test mode
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

// Toggle LED test mode
void toggle_led_test() {
    if (led_test) {
        all_leds_off();
        led_test = 0;
        pwm_active = 1;     // Re-enable PWM
    } else {
        pwm_active = 0;     // Disable PWM
        led_test = 1;
        all_leds_on();
    }
}

// Startup sequence when clock powers on
void startup_sequence() {
    for (int i = 7; i >= 3; i--) {
        PORTD |= (1 << i);
        _delay_ms(50);
        PORTD &= ~(1 << i);
    }

    for (int i = 3; i < 8; i++) {
        PORTD |= (1 << i);
        _delay_ms(50);
        PORTD &= ~(1 << i);
    }

    for (int i = 5; i >= 0; i--) {
        PORTC |= (1 << i);
        _delay_ms(50);
        PORTC &= ~(1 << i);
    }

    for (int i = 0; i < 6; i++) {
        PORTC |= (1 << i);
        _delay_ms(50);
        PORTC &= ~(1 << i);
    }

    _delay_ms(300);
}

void sleep_mode_sequence() {
    all_leds_off();
    for (int i = 0; i < 6; i++) {
        PORTC |= (1 << i);
        PORTD |= (1 << i+3);
        _delay_ms(200);
        PORTC &= ~(1 << i);
        PORTD &= ~(1 << i+3);
    }
}

void wakeup_sequence(){
    all_leds_off();
    for (int i = 5; i >= 0; i--) {
        PORTC |= (1 << i);
        PORTD |= (1 << i+3);
        _delay_ms(200);
        PORTC &= ~(1 << i);
        PORTD &= ~(1 << i+3);
    }
}

// Debounce function for buttons at PB0 & PB1
uint8_t debounce_button_b(uint8_t button) {
    _delay_ms(50);
    return !(PINB & button);
}

// Debounce function for button at PD2
uint8_t debounce_button_d(uint8_t button) {
    _delay_ms(50);
    return !(PIND & button);
}

// Turn on all LEDs
void all_leds_on() {
    PORTC |= MINUTE_LEDS; //00111111 - Minute LEDs
    PORTD |= HOUR_LEDS; //11111000 - Hour LEDs
}

// Turn off all LEDs
void all_leds_off() {
    PORTC &= ~MINUTE_LEDS; //00111111 - Minute LEDs
    PORTD &= ~HOUR_LEDS; //11111000 - Hour LEDs
}
