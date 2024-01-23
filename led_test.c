#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>

void main() {
	DDRC = (1<<5);
	uint8_t time = 100;
	while (1) {

		DDRC = (0x3F);//00011111
		DDRD = (0xF8);//11111000

		//C0
		PORTC|=(1<<0);
		_delay_ms(time);
		PORTC&=~(1<<0);

		//C1
		PORTC|=(1<<1);
		_delay_ms(time);
		PORTC&=~(1<<1);
		
		//C2
		PORTC|=(1<<2);
		_delay_ms(time);
		PORTC&=~(1<<2);

		//C3
		PORTC|=(1<<3);
		_delay_ms(time);
		PORTC&=~(1<<3);

		//C4
		PORTC|=(1<<4);
		_delay_ms(time);
		PORTC&=~(1<<4);
		
		//C5
		PORTC|=(1<<5);
		_delay_ms(time);
		PORTC&=~(1<<5);
		
		//D3
		PORTD|=(1<<3);
		_delay_ms(time);
		PORTD&=~(1<<3);
		
		//D4
		PORTD|=(1<<4);
		_delay_ms(time);
		PORTD&=~(1<<4);
		
		//D5
		PORTD|=(1<<5);
		_delay_ms(time);
		PORTD&=~(1<<5);
		
		//D6
		PORTD|=(1<<6);
		_delay_ms(time);
		PORTD&=~(1<<6);
		
		//D7
		PORTD|=(1<<7);
		_delay_ms(time);
		PORTD&=~(1<<7);
		
		_delay_ms(500);
        allLedsOn();
		_delay_ms(500);
        allLedsOff();
		_delay_ms(500);
		}
}

void allLedsOn(){
	PORTC|=(1<<0);
	PORTC|=(1<<1);
	PORTC|=(1<<2);
	PORTC|=(1<<3);
	PORTC|=(1<<4);
	PORTC|=(1<<5);
	PORTD|=(1<<3);
	PORTD|=(1<<4);
	PORTD|=(1<<5);
	PORTD|=(1<<6);
	PORTD|=(1<<7);
}

void allLedsOff(){
	PORTC&=~(1<<0);
	PORTC&=~(1<<1);
	PORTC&=~(1<<2);
	PORTC&=~(1<<3);
	PORTC&=~(1<<4);
	PORTC&=~(1<<5);
	PORTD&=~(1<<3);
	PORTD&=~(1<<4);
	PORTD&=~(1<<5);
	PORTD&=~(1<<6);
	PORTD&=~(1<<7);
}
