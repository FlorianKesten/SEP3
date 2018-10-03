#include "RTOS_utils.h"

void prepare_shiftregister()
{
	// Set SER to 1
	PORTD |= _BV(PORTD2);
}

// clock shift-register
void clock_shift_register_and_prepare_for_next_col()
{
	// one SCK pulse
	PORTD |= _BV(PORTD5);
	PORTD &= ~_BV(PORTD5);
	
	// one RCK pulse
	PORTD |= _BV(PORTD4);
	PORTD &= ~_BV(PORTD4);
	
	// Set SER to 0 - for next column
	PORTD &= ~_BV(PORTD2);
}

// Load column value for column to show
void load_col_value(uint16_t col_value)
{
	PORTA = ~(col_value & 0xFF);
	
	// Manipulate only with PB0 and PB1
	PORTB |= 0x03;
	PORTB &= ~((col_value >> 8) & 0x03);
}

//-----------------------------------------
void vApplicationIdleHook( void )
{
	//
}

//method used to calculate the power of number with base, and power - p

int joystick_func()
{
	uint16_t output = PINC;
	uint16_t mask = 0b11000011;
	return output & mask; // | is supposed to be used for "OR"
}

int button_func()
{
	uint16_t output = PIND;
	uint16_t mask = 0b11111111;
	return output & mask; // | is supposed to be used for "OR"
}

int key_received()
{
	BaseType_t result = 0;
	uint8_t byte;
	
	result = xQueueReceive(_x_com_received_chars_queue, &byte, 1000L);
	if(result==0){
		return 0;
	}
	else{
		return byte;
	}
}