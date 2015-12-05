/***********************************************************
 * piano3d_R2.c 
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
 * cathode, within the keyboard) necessitate AN0-5 to be 
 * pulled high and read as CAT0-10 are sunk in progression.
 *
 * I/O lines ANx are pulled high to Vcc with 10k resistors
 * and also decoupled to Gnd with 220pF capacitors.
 **********************************************************/

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>

#define CALBYTE	0x83;		// OSCCAL byte value
#define F_CPU	8000000UL	// Clock speed (8MHz CKSEL3..0=0010)
#include <util/delay.h>

//#define LEDTEST   1		// test with LEDs (no UART) 

#define NUMKEYS  61		// 61 piano keys
#define ANODES    6		//  6 anodes
#define CATHODES 11		// 11 cathodes

#define TESTODE   5		// cat# to test for LEDTEST

// Flow control
#define RTS_n	(PIND & (1<<PIND2))	// Input from FTDI chip
//#define CTS_n	PORTD.PD3		// Output to FTDI chip
#define CTS_n_mask	0xF7		// & to send pd3 low

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

typedef unsigned long long KEYS;	// keylist data type

/* GLOBALS */
char flag = 0;
KEYS keylist;			// list of 61 keys (1 bit per key)
/* ******* */


void USART_Transbyte(unsigned char data){
	while ( !(UCSR0A & (0x20)) );  /* Wait for empty transmit buffer */
	UDR0 = data;  /* Put data into buffer, sends the data */
}  

void USART_Transword(unsigned int data){
	USART_Transbyte((char) ((data>>8) & 0xFF));     // send high byte
	USART_Transbyte((char) (data & 0xFF));          // send low byte
}


void sendkeylist(unsigned int packet){
	KEYS data;		//temp var
	data = keylist;		//grab global values as-is

	USART_Transword(packet); 		 // send packet# - 2bytes

	USART_Transbyte((char) (data & 0xFF));   // send keydata - 1byte (LSB)
	data >>= 8;
	USART_Transbyte((char) (data & 0xFF));	 // byte #2
	data >>= 8;
	USART_Transbyte((char) (data & 0xFF));	 // byte #3
	data >>= 8;
	USART_Transbyte((char) (data & 0xFF));	 // byte #4

	data >>= 8;
	USART_Transbyte((char) (data & 0xFF));	 // byte #5
	data >>= 8;
	USART_Transbyte((char) (data & 0xFF));	 // byte #6
	data >>= 8;
	USART_Transbyte((char) (data & 0xFF));	 // byte #7
	data >>= 8;
	USART_Transbyte((char) (data & 0xFF));	 // byte #8		 (MSB)

	/* Total Packet Size: 10 bytes */
}


void getkeylist(void){

	/* Set bits of keys 1 or 0 if pressed or released */
	unsigned char bufbyte, bufbyte2;
#ifdef LEDTEST
       	unsigned char ledbuf;
#endif
	unsigned int i;
	KEYS data;		// temp var for keylist
	KEYS shifter;		// var for shifting mask

	/* Read AN0-5 as CAT0-10 are pulled low in succession */
	/* During LEDTEST, LEDs are considered on PortD, whereas */
	/* in normal ops, Tx/Rx pins are considered on PortD */

	data   = 0x0000000000000000;		// assume all keys NOT pressed
	shifter= 0x0000000000000001;		// shifting a 1 for masking

#ifndef LEDTEST
	/* FULL Keyboard Read */
	i = 0;  bufbyte = 0xFF;  PORTB = bufbyte & ~(1<<i);
	_delay_us(2);
	if(!AN0)    data = shifter;		// Special case @ i=0
	for(i=1; i<CATHODES; i++){		// Start at 1 to avoid special case
	  if(i<8){
		bufbyte  = 0xFF;		// default portB config
		PORTB = bufbyte  & ~(1<<i);     // send low one cathode group
	  }
	  else{
		bufbyte2 = PIND | 0xE0;		// portD Rx/Tx, Flow Ctrl pins considered, adjust pd5,6,7
		PORTD = bufbyte2 & ~(1<<(i-3));	// pd5,6,7 for CAT8,9,10
	  }

	  _delay_us(5);			// NEED this delay here! (time to pull cathode low)
	  if(!AN0)  data += (shifter<<(i*ANODES-0));	// cathode_index * 6_anodes_per - anode_num
	  if(!AN1)  data += (shifter<<(i*ANODES-1));	//
	  if(!AN2)  data += (shifter<<(i*ANODES-2));	//
	  if(!AN3)  data += (shifter<<(i*ANODES-3));	//
	  if(!AN4)  data += (shifter<<(i*ANODES-4));	//
	  if(!AN5)  data += (shifter<<(i*ANODES-5));	//
	}
#else
	/* FULL Keyboard test with LEDs */
	ledbuf=0xFF;				// buffer for LEDs
	i = 0;  bufbyte = 0xFF;  PORTB = bufbyte & ~(1<<i);
	if(!AN0)    data = 1;			// Special case @ i=0
	for(i=1; i<CATHODES; i++){		// Start at 1 to avoid special case
	  if(i<8){
		bufbyte  = 0xFF;		// default portB config
		PORTB = bufbyte  & ~(1<<i);	// send low one cathode group
	  }
	  else{
		bufbyte2 = PIND & 0xFF;		// portD leds considered, adjust pd5,6,7
		PORTD = bufbyte2 & ~(1<<(i-3));	// pd5,6,7 for CAT8,9,10
	  }

	  _delay_us(3);			// NEED this delay here! (time to pull cathode low)
	  if(!AN0){  data += (1<<(i*ANODES-0));	ledbuf &= 0xFE; } // cathode_index * 6_anodes_per - anode_num
	  if(!AN1){  data += (1<<(i*ANODES-1));	ledbuf &= 0xFD;	} // and set leds...
	  if(!AN2){  data += (1<<(i*ANODES-2));	ledbuf &= 0xFB;	} //
	  if(!AN3){  data += (1<<(i*ANODES-3));	ledbuf &= 0xF7;	} //
	  if(!AN4){  data += (1<<(i*ANODES-4));	ledbuf &= 0xEF;	} //
	  if(!AN5){  data += (1<<(i*ANODES-5));	}		  // ignore 6th (only 5 leds)
	}
#endif

	keylist = data;			// set key list

	/* RESET CATHODES */
	PORTB = 0xFF;
#ifndef LEDTEST
	PORTD = PIND | 0xE0;		// 'OR' due to tx/rx, flow ctrl pins
#else
	//PORTD = PIND & 0xFF;		// 'AND' due to active low LEDs
	PORTD = ledbuf;			// write LEDs to port
#endif

}


void init(void){

// Internal RC Oscillator
#ifdef CALBYTE
OSCCAL=CALBYTE;	 // tune
#endif
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
// Func7=Out Func6=Out Func5=Out Func4=In Func3=Out Func2=In Func1=In Func0=In
// State7=1  State6=1  State5=1  State4=T State3=1  State2=P State1=T State0=T 
#ifndef LEDTEST
PORTD=0xE4;
DDRD=0xE8;
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
UBRR0L=0x19;	// @ 8MHz: 0x33 for 9600baud, 0x19 for 19.2k, 0x0C for 38.4k, 0x01 for 250k, 0x00 for 0.5M
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

  unsigned int i;
  keylist = 0x0000000000000000;	// long long is 8 bytes

  asm volatile("sei"::);	// Global enable interrupts
  flag=1;			// move to timer interrupt?
  i=0;				// init packet counter

#ifndef LEDTEST
    PORTD = PIND & CTS_n_mask;	// assert clear to send
#endif
  while(1){

    _delay_us(200);	// check keys this often... or use flag
    getkeylist();
#ifndef LEDTEST
    //PORTD = PIND & CTS_n_mask;	// assert clear to send
    if(!RTS_n){				// if RTS asserted, send
      PORTD = PIND | 0x08;		// de-assert clear to send
      sendkeylist(i++);			//
      PORTD = PIND & CTS_n_mask;	// assert clear to send
    }
#endif

  }

  return 0;

}

