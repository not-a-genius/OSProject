#include "IRremote.h"
#include "IRremoteInt.h"
#include "Functions.h"
#include "pins_arduino.h"
#include "wiring_private.h"
#include "MyArduino.h"

#include <stdio.h>
#include <stdlib.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define HEX 16
#define RECV_PIN 8//questo è proprio il pin! ( D11)
#define RECV_PINX PINB0	
#define RECV_DDX DDB0
#define FNV_PRIME_32 16777619
#define FNV_BASIS_32 2166136261

/*	BIT MATH OPERATIONS MACRO */
//get the bit in bit_pos position in reg register
#define bit_get(reg,bit_pos) ((reg) & (1<<(bit_pos) ))	
//set the bit in bit_pos position in reg register
#define bit_set(reg,bit_pos) ((reg) |= (1<<(bit_pos) ))	
//clear the bit in bit_pos position in reg register
#define bit_clear(reg,bit_pos) ((reg) &= ~(1<<(bit_pos) ))	
//flip the bit in bit_pos position in reg register
#define bit_flip(reg,bit_pos) ((reg) ^= (1<<(bit_pos) ))	
//check if the bit in bit_pos in the reg register is one (1) or zero (0)
#define bit_is_one(reg,bit_pos) (( (reg) & (1<<(bit_pos) ) )> 0)

/* IDE SCHEMA
/////////////////////////////////
/////////////////////////////////
#include <IRremote.h>

int RECV_PIN = 11;
IRrecv irrecv(RECV_PIN);
decode_results results;

void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
}

void loop() {
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    irrecv.resume(); // Receive the next value
  }
}

/////////////////////////////////
/////////////////////////////////
*/



volatile irparams_t irparams;

void resume() {
  irparams.rcvstate = STATE_IDLE;
  irparams.rawlen = 0;
}

void enableIRIn() {
  cli();
  // setup pulse clock timer interrupt
  //Prescale /8 (16M/8 = 0.5 microseconds per tick)
  // Therefore, the timer interval can range from 0.5 to 128 microseconds
  // depending on the reset value (255 to 0)
  TIMER_CONFIG_NORMAL();

  //Timer2 Overflow Interrupt Enable
  TIMER_ENABLE_INTR;

  TIMER_RESET;

  sei();  // enable interrupts

  // initialize state machine variables
  irparams.rcvstate = STATE_IDLE;
  irparams.rawlen = 0;
  
  irparams.recvpin=RECV_PIN;	


  // set pin modes
  //pinMode(irparams.recvpin, INPUT);	
  //set pin recvpin as input with pull up
  
	/*
	DDRB &= ~(1 << RECV_PIN); // Clear the PB3 pin
    // PB3 is now an input
	
	PORTB |= (1 << RECV_PIN); // turn On the Pull-up
    // PB3 is now an input with pull-up enabled
	*/
	
	DDRB &= ~(1<<RECV_DDX); // Clear the PB0 pin che ora è un input
	
	
}

int decode(decode_results *results) {
  results->rawbuf = irparams.rawbuf;
  results->rawlen = irparams.rawlen;
  if (irparams.rcvstate != STATE_STOP) {
    return ERR;
  }
  
  //TODO sostituire tutti i serial.println con usart...
#ifdef DEBUG
  Serial.println("Attempting NEC decode");
#endif
  if (decodeNEC(results)) {
    return DECODED;
  }
#ifdef DEBUG
  Serial.println("Attempting Sony decode");
#endif
  if (decodeSony(results)) {
    return DECODED;
  }
#ifdef DEBUG
  Serial.println("Attempting Sanyo decode");
#endif
  if (decodeSanyo(results)) {
    return DECODED;
  }
#ifdef DEBUG
  Serial.println("Attempting Mitsubishi decode");
#endif
  if (decodeMitsubishi(results)) {
    return DECODED;
  }
#ifdef DEBUG
  Serial.println("Attempting RC5 decode");
#endif  
  if (decodeRC5(results)) {
    return DECODED;
  }
#ifdef DEBUG
  Serial.println("Attempting RC6 decode");
#endif 
  if (decodeRC6(results)) {
    return DECODED;
  }
#ifdef DEBUG
    Serial.println("Attempting Panasonic decode");
#endif 
    if (decodePanasonic(results)) {
        return DECODED;
    }
#ifdef DEBUG
    Serial.println("Attempting LG decode");
#endif 
    if (decodeLG(results)) {
        return DECODED;
    }
#ifdef DEBUG
    Serial.println("Attempting JVC decode");
#endif 
    if (decodeJVC(results)) {
        return DECODED;
    }
#ifdef DEBUG
  Serial.println("Attempting SAMSUNG decode");
#endif
  if (decodeSAMSUNG(results)) {
    return DECODED;
  }
  // decodeHash returns a hash on any input.
  // Thus, it needs to be last in the list.
  // If you add any decodes, add them before this.
  if (decodeHash(results)) {
    return DECODED;
  }
  // Throw away and start over
  resume();
  return ERR;
}

int main(void){
	
	decode_results results;
	
	//setup
	char hello_str[]="ready to receive\n";
	char decoded_buffer [sizeof(unsigned long)*8+1];
	 
	 
	enableIRIn();
	cli();	
	USART_init();
	USART_putstring(hello_str);
	sei();
	
	
	//loop
	while(1){
			//digital read testing 
		#if MYTEST
			  uint8_t dig_read = (uint8_t)(PINB & _BV(3)) == 0; // digitalRead (11);	//bloccante ?
			  cli();
			  USART_send('a');	//non va 
			  sei();
		#else
		
		 if ( decode(&results) ) {
			//Serial.println(results.value, HEX);
						
			ultoa(results.value, decoded_buffer, HEX); //pass results value into a string
			cli();
			//USART_putstring("Received:\n");
			USART_putstring(decoded_buffer);
			USART_send('\n');
			sei();
			resume(); // Receive the next value
		}
		
		#endif
			
	    _delay_ms(100);
	}
	
		
	return 0;
	
}

////////////////////////////	/////////////////////////////////////////////////////////7777   TO COPY DIGITAL READ

static void turnOffPWM(uint8_t timer)
{
	switch (timer)
	{
		#if defined(TCCR1A) && defined(COM1A1)
		case TIMER1A:   cbi(TCCR1A, COM1A1);    break;
		#endif
		#if defined(TCCR1A) && defined(COM1B1)
		case TIMER1B:   cbi(TCCR1A, COM1B1);    break;
		#endif
		#if defined(TCCR1A) && defined(COM1C1)
		case TIMER1C:   cbi(TCCR1A, COM1C1);    break;
		#endif
		
		#if defined(TCCR2) && defined(COM21)
		case  TIMER2:   cbi(TCCR2, COM21);      break;
		#endif
		
		#if defined(TCCR0A) && defined(COM0A1)
		case  TIMER0A:  cbi(TCCR0A, COM0A1);    break;
		#endif
		
		#if defined(TCCR0A) && defined(COM0B1)
		case  TIMER0B:  cbi(TCCR0A, COM0B1);    break;
		#endif
		#if defined(TCCR2A) && defined(COM2A1)
		case  TIMER2A:  cbi(TCCR2A, COM2A1);    break;
		#endif
		#if defined(TCCR2A) && defined(COM2B1)
		case  TIMER2B:  cbi(TCCR2A, COM2B1);    break;
		#endif
		
		#if defined(TCCR3A) && defined(COM3A1)
		case  TIMER3A:  cbi(TCCR3A, COM3A1);    break;
		#endif
		#if defined(TCCR3A) && defined(COM3B1)
		case  TIMER3B:  cbi(TCCR3A, COM3B1);    break;
		#endif
		#if defined(TCCR3A) && defined(COM3C1)
		case  TIMER3C:  cbi(TCCR3A, COM3C1);    break;
		#endif

		#if defined(TCCR4A) && defined(COM4A1)
		case  TIMER4A:  cbi(TCCR4A, COM4A1);    break;
		#endif					
		#if defined(TCCR4A) && defined(COM4B1)
		case  TIMER4B:  cbi(TCCR4A, COM4B1);    break;
		#endif
		#if defined(TCCR4A) && defined(COM4C1)
		case  TIMER4C:  cbi(TCCR4A, COM4C1);    break;
		#endif			
		#if defined(TCCR4C) && defined(COM4D1)
		case TIMER4D:	cbi(TCCR4C, COM4D1);	break;
		#endif			
			
		#if defined(TCCR5A)
		case  TIMER5A:  cbi(TCCR5A, COM5A1);    break;
		case  TIMER5B:  cbi(TCCR5A, COM5B1);    break;
		case  TIMER5C:  cbi(TCCR5A, COM5C1);    break;
		#endif
	}
}

extern const uint16_t PROGMEM port_to_mode_PGM[];
extern const uint16_t PROGMEM port_to_input_PGM[];
extern const uint16_t PROGMEM port_to_output_PGM[];

extern const uint8_t PROGMEM digital_pin_to_port_PGM[];
// extern const uint8_t PROGMEM digital_pin_to_bit_PGM[];
extern const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[];
extern const uint8_t PROGMEM digital_pin_to_timer_PGM[];

// Get the bit location within the hardware port of the given virtual pin.
// This comes from the pins_*.c file for the active board configuration.
// 
// These perform slightly better as macros compared to inline functions
//
//



void pinMode(uint8_t pin, uint8_t mode)
{
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	volatile uint8_t *reg, *out;

	if (port == NOT_A_PIN) return;

	// JWS: can I let the optimizer do this?
	reg = portModeRegister(port);
	out = portOutputRegister(port);

	if (mode == INPUT) { 
		uint8_t oldSREG = SREG;
                cli();
		*reg &= ~bit;
		*out &= ~bit;
		SREG = oldSREG;
	} else if (mode == INPUT_PULLUP) {
		uint8_t oldSREG = SREG;
                cli();
		*reg &= ~bit;
		*out |= bit;
		SREG = oldSREG;
	} else {
		uint8_t oldSREG = SREG;
                cli();
		*reg |= bit;
		SREG = oldSREG;
	}
}

int digitalRead(uint8_t pin)
{
	uint8_t timer = digitalPinToTimer(pin);
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);

	if (port == NOT_A_PIN) return LOW;

	// If the pin that support PWM output, we need to turn it off
	// before getting a digital reading.
	if (timer != NOT_ON_TIMER) turnOffPWM(timer);

	if (*portInputRegister(port) & bit) return HIGH;
	return LOW;
}

////////////////////////////////////////////////////////////////  INTERRUPT MANAGEMENT




#if ! MYTEST

ISR(TIMER_INTR_NAME)
{
  TIMER_RESET;

  //uint8_t irdata = (uint8_t)digitalRead(irparams.recvpin);	// <- cosi non va 

  uint8_t irdata = (uint8_t)(PINB & (1<<RECV_PINX) ) == 0; // digitalRead (11);

  irparams.timer++; // One more 50us tick
  if (irparams.rawlen >= RAWBUF) {
    // Buffer overflow
    irparams.rcvstate = STATE_STOP;
  }
  switch(irparams.rcvstate) {
  case STATE_IDLE: // In the middle of a gap
    if (irdata == MARK) {
      if (irparams.timer < GAP_TICKS) {
        // Not big enough to be a gap.
        irparams.timer = 0;
      } 
      else {
        // gap just ended, record duration and start recording transmission
        irparams.rawlen = 0;
        irparams.rawbuf[irparams.rawlen++] = irparams.timer;
        irparams.timer = 0;
        irparams.rcvstate = STATE_MARK;
      }
    }
    break;
  case STATE_MARK: // timing MARK
    if (irdata == SPACE) {   // MARK ended, record time
      irparams.rawbuf[irparams.rawlen++] = irparams.timer;
      irparams.timer = 0;
      irparams.rcvstate = STATE_SPACE;
    }
    break;
  case STATE_SPACE: // timing SPACE
    if (irdata == MARK) { // SPACE just ended, record it
      irparams.rawbuf[irparams.rawlen++] = irparams.timer;
      irparams.timer = 0;
      irparams.rcvstate = STATE_MARK;
    } 
    else { // SPACE
      if (irparams.timer > GAP_TICKS) {
        // big SPACE, indicates gap between codes
        // Mark current code as ready for processing
        // Switch to STOP
        // Don't reset timer; keep counting space width
        irparams.rcvstate = STATE_STOP;
      } 
    }
    break;
  case STATE_STOP: // waiting, measuring gap
    if (irdata == MARK) { // reset gap timer
      irparams.timer = 0;
    }
    break;
  }

  if (irparams.blinkflag) {
    if (irdata == MARK) {
      BLINKLED_ON();  // turn pin 13 LED on
    } 
    else {
      BLINKLED_OFF();  // turn pin 13 LED off
    }
  }
}

#endif

