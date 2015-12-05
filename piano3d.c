/***********************************************************
 * piano3d_R0.c 
 *
 * Tyler Niles
 * 4/15/2010
 *
 * VR CSCI6830 - Mixed Reality Project
 *
 * This file writen for use on the ATMEGA168 using
 * the GNU AVR-GCC compiler. 
 *
 * Piano keyboard is a 61-key YAMAHA PSR-200 and is scanned 
 * and read via a 17-conductor ribbon cable with AN0-5 and 
 * CAT0-10. Conductors are as follows:
 *
 * C1-AN0, C2-AN5, C3-AN1, C5-AN2, C7-AN3, C9-AN4,
 *
 * C4-CAT6, C6-CAT5, C8-CAT7, C10-CAT8, C11-CAT1, C12-CAT9,
 * C13-CAT2, C14-CAT10, C15-CAT3, C16-CAT0, C17-CAT4.
 *
 * Diodes placed in series with the switches (from anode to
 * cathode) necessitate AN0-5 to be pulled high and read as
 * CAT0-10 are sunk in progression.
 **********************************************************/

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>

#define F_CPU	8000000UL	// Clock speed
#include <util/delay.h>

//#define LEDTEST   1		// test with LEDs (no UART) 

#define NUMKEYS  61		// 61 piano keys
#define ANODES    6		//  6 anodes
#define CATHODES 11		// 11 cathodes

#define TESTODE   5		// cat# to test for LEDTEST

// Read these Pins
#define AN0	(PINC & (1<<PINC0))	//c1
#define AN1	(PINC & (1<<PINC2))	//c3
#define AN2	(PINC & (1<<PINC3))	//c5
#define AN3	(PINC & (1<<PINC4))	//c7
#define AN4	(PINC & (1<<PINC5))	//c9
#define AN5	(PINC & (1<<PINC1))	//c2
// Write to Ports
/*
//PortB		For Reference...(more efficient w/o defines)
#define CAT0	PORTB.PB0	//c16
#define CAT1	PORTB.PB1	//c11
#define CAT2	PORTB.PB2	//c13
#define CAT3	PORTB.PB3	//c15
#define CAT4	PORTB.PB4	//c17
#define CAT5	PORTB.PB5	//c 6
#define CAT6	PORTB.PB6	//c 4
#define CAT7	PORTB.PB7	//c 8
//PortD
#define CAT8	PORTD.PD5	//c10
#define CAT9	PORTD.PD6	//c12
#define CAT10	PORTD.PD7	//c14
*/

typedef unsigned char KEYS;	// keylist data type

/* GLOBALS */
char flag = 0;


void USART_Transbyte(unsigned char data){
	while ( !(UCSR0A & (0x20)) );  /* Wait for empty transmit buffer */
	UDR0 = data;  /* Put data into buffer, sends the data */
}  

void USART_Transword(unsigned int data){
	USART_Transbyte((char) ((data>>8) & 0xFF));     // send high byte
	USART_Transbyte((char) (data & 0xFF));          // send low byte
}
  
void send(unsigned int value, unsigned char data){
    USART_Transword(value);  	// send key# - 2byte
    USART_Transbyte(data);	// send keydata - 1bytes
    USART_Transbyte('\r');      // CR terminator: decimal 13      
    // Total Packet Size: 4 bytes
}

void getkeys(KEYS keys[NUMKEYS]){

	/* Set list of keys 1 or 0 if pressed or released */
	unsigned char bufbyte, bufbyte2;
	unsigned int i;

	/* Read AN0-5 as CAT0-10 are pulled low in succession */
	/* During LEDTEST, LEDs are considered on PortD, whereas */
	/* in normal ops, Tx/Rx pins are considered on PortD */

#ifndef LEDTEST
	/* FULL Keyboard Read */
	i = 0;  bufbyte  = 0xFF;  PORTB = bufbyte  & ~(1<<i);
	if(!AN0)     keys[0] = 1;	// Special case @ i=0
	for(i=1; i<CATHODES; i++){	// Start at 1 to avoid special case
	  bufbyte  = 0xFF;		// default portB config
	  bufbyte2 = PIND | 0xE0;	// portD Rx/Tx pins considered, adjust pd5,6,7
	  if(i<8)    PORTB = bufbyte  & ~(1<<i);    // send low one cathode group
	  else	     PORTD = bufbyte2 & ~(1<<(i-3));	// pd5,6,7 for CAT8,9,10

	  _delay_us(3);	// NEED this delay here! (time for pulling cathode low)
	  keys[i*ANODES-0] = AN0 ? 0:1;	// cathode_index * 6_anodes_per - anode_num
	  keys[i*ANODES-1] = AN1 ? 0:1;	// if sent low, make 1 b/c key is pressed
	  keys[i*ANODES-2] = AN2 ? 0:1;	// 
	  keys[i*ANODES-3] = AN3 ? 0:1;	// 
	  keys[i*ANODES-4] = AN4 ? 0:1;	// 
	  keys[i*ANODES-5] = AN5 ? 0:1;	// 
	}
#else
/*
	// SINGLE cathode group test 
	i = TESTODE;			// 5th group of cathodes (has middle C sharp)
	bufbyte  = 0xFF;		// default portB config
	bufbyte2 = PIND & 0xFF;		// portD leds considered, adjust pd5,6,7
	if(i<8)    PORTB = bufbyte  & ~(1<<i);    // send low one cathode group
	else       PORTD = bufbyte2 & ~(1<<(i-3));	// pd5,6,7 for CAT8,9,10

	_delay_us(1);	// NEED this delay here! (time for pulling cathode low)
	keys[i*ANODES-0] = AN0 ? 0:1;	// cathode_index * 6_anodes_per - anode_num
	keys[i*ANODES-1] = AN1 ? 0:1;	// if sent low, make 1 b/c key is pressed
	keys[i*ANODES-2] = AN2 ? 0:1;	// 
	keys[i*ANODES-3] = AN3 ? 0:1;	// 
	keys[i*ANODES-4] = AN4 ? 0:1;	// 
	keys[i*ANODES-5] = AN5 ? 0:1;	// 
*/
	// FULL Keyboard test 
	i = 0;  bufbyte  = 0xFF;  PORTB = bufbyte  & ~(1<<i);
	if(!AN0)     keys[0] = 1;	// Special case @ i=0
	for(i=1; i<CATHODES; i++){	// Start at 1 to avoid special case
	  bufbyte  = 0xFF;		// default portB config
	  bufbyte2 = PIND & 0xFF;	// portD leds considered, adjust pd5,6,7
	  if(i<8)    PORTB = bufbyte  & ~(1<<i);    // send low one cathode group
	  else	     PORTD = bufbyte2 & ~(1<<(i-3));	// pd5,6,7 for CAT8,9,10

	  _delay_us(3);	// NEED this delay here! (time for pulling cathode low)
	  keys[i*ANODES-0] = AN0 ? 0:1;	// cathode_index * 6_anodes_per - anode_num
	  keys[i*ANODES-1] = AN1 ? 0:1;	// if sent low, make 1 b/c key is pressed
	  keys[i*ANODES-2] = AN2 ? 0:1;	// 
	  keys[i*ANODES-3] = AN3 ? 0:1;	// 
	  keys[i*ANODES-4] = AN4 ? 0:1;	// 
	  keys[i*ANODES-5] = AN5 ? 0:1;	// 
	}
#endif

	/* RESET CATHODES */
	PORTB = 0xFF;
#ifndef LEDTEST
	PORTD = PIND | 0xE0;		// 'OR' due to tx/rx pins
#else
	PORTD = PIND & 0xFF;		// 'AND' due to active low LEDs
#endif

}

void sendkeylist(KEYS keys[NUMKEYS]){
	unsigned int i;
	//for(i=0; i<NUMKEYS; i++){
	for(i=0; i<1; i++){
		send(i,keys[i]);
	}
}

void lightleds(KEYS keys[NUMKEYS]){
	/* used to test keypad function */ 
	/* (LEDS on PD0-4, sacrifice 6th key during testing) */
	unsigned char bufbyte;
	unsigned int i=TESTODE;	// 5th cathode group
	bufbyte  = 0xFF;	// init
	/* FULL Keyboard test */
	for(i=1; i<CATHODES; i++){	// skip special case
	  bufbyte &= ~( (keys[i*ANODES-4]<<PD4) | (keys[i*ANODES-3]<<PD3) | (keys[i*ANODES-2]<<PD2) | \
		        (keys[i*ANODES-1]<<PD1) | (keys[i*ANODES-0]<<PD0) );  // set for keys pressed
	}
	PORTD = bufbyte;	// write to port
}

void init(void){

// Crystal Oscillator
CLKPR=0x80;
CLKPR=0x00;  // set clock div factor to 1

// Port B
// Func7=Out Func6=Out Func5=Out Func4=Out Func3=Out Func2=Out Func1=Out Func0=Out
// State7=1 State6=1 State5=1 State4=1 State3=1 State2=1 State1=1 State0=1 
PORTB=0xFF;
DDRB=0xFF;

// Port C
// Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
PORTC=0x00;
DDRC=0x00;

// Port D
// Func7=Out Func6=Out Func5=Out Func4=In Func3=In Func2=In Func1=In Func0=In
// State7=1 State6=1 State5=1 State4=T State3=T State2=T State1=T State0=T 
#ifndef LEDTEST
PORTD=0xE0;
DDRD=0xE0;
#else
PORTD=0xFF;
DDRD=0xFF;
#endif

// USART
// Communication Parameters: 8 Data, 1 Stop, No Parity
// USART Receiver	   : ON
// USART Transmitter	   : ON
// USART0 Mode		   : Asynchronous
// USART Baud Rate	   : 250k
#ifndef LEDTEST
UCSR0A=0x00;
UCSR0B=0x18;	//enable tx/rx
UCSR0C=0x06;
UBRR0H=0x00;
UBRR0L=0x01;	// @ 8MHz: 0x33 for 9600baud, 0x19 for 19.2k, 0x0C for 38.4k, 0x01 for 250k, 0x00 for 0.5M
#else
UCSR0A=0x00;
UCSR0B=0x00;
UCSR0C=0x06;
UBRR0H=0x00;
UBRR0L=0x33;
#endif

// Analog Comparator
// Analog Comparator: OFF
// Analog Comparator Input Capture by Timer/Counter 1: OFF
ACSR=0x00;
ACSR=0x80;	// disable ac
ADCSRB=0x00;

// ADC initialization
// ADC: OFF
DIDR0=0x00;
ADMUX=0x00;
ADCSRA=0x03;

}

int main(void){

  asm volatile("cli"::);	// Global disable interrupts
  init();
  KEYS keys[NUMKEYS];	// list of 61 keys
  unsigned int i;
  for(i=0;i<NUMKEYS;i++){ keys[i] = 0; }
  asm volatile("sei"::);	// Global enable interrupts
  flag=1;			// move to timer interrupt?

  while(1){
    _delay_us(200);	//check keys this often... or use flag
    getkeys(keys);
#ifndef LEDTEST
    sendkeylist(keys);
#else
    lightleds(keys);
#endif
  }

  return 0;
}

