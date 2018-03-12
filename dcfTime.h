#ifndef DCFTIME_h
#define DCFTIME_h

/* dcfTime, a "Proof of Concept" class to decode and parse DCF77 time data.
 * Adapted from the Arduino msfTime library Copyright 2014 Phil Morris
 * 
 * Version 3.0.0 (beta)
 * Copyright 2017 Phil Morris <www.lydiard.plus.com>
 * 
 * This software is in the Public Domain and can be used,
 * modified and copied as long as this text remains intact.
 * 
 * CHANGES FROM VERSION 2
 *
 * The original versions did all the decoding and processing within the DCF
 * pulse ISR. This version splits the main decoding function off into a method
 * "timeAvailable()" which is called from the loop of the sketch.
*/

#define DCF_DEBUG true		// DEBUGGING enable true/false. This outputs the bit values of the received pulses

#ifndef Arduino_h
	#include <Arduino.h>
#endif

#include <TimeLib.h>

#define DCF_NO_PIN -1			// a value of -1 will result in no action from pinMode etc.

// user changeable values
#define DCF_PULSE_INPUT_PIN 				DCF_NO_PIN	// the EXTERNAL interrupt pin for DCF receiver pulses
#define DCF_PULSE_CARRIER_OFF_LEVEL HIGH				// the RX output when the carrier goes OFF
#define DCF_PULSE_EXTEND_TIME				5						// a padding value in milliseconds
#define DCF_LED_PIN									DCF_NO_PIN	// optional LED output pin
#define DCF_LED_ON_LEVEL						HIGH				// the level needed to switch the LED ON
#define DCF_PON_PIN									DCF_NO_PIN	// optional PON output pin
#define DCF_PON_ON_LEVEL						LOW					// the level needed to switch the RX ON
#define DCF_MIN_PULSE 							100					// the minimum pulse length acceptable in milliseconds
#define DCF_MAX_PULSE								220					// the maximum pulse length acceptable in milliseconds
#define DCF_MIN_SECOND							950					// a value to ensure that ~1 second has passed

// DCF time value offsets and lengths
#define DCF_TIME_START_BIT	20
#define DCF_MINUTE_START	21
#define DCF_MINUTE_LEN		7
#define DCF_HOUR_START		29
#define DCF_HOUR_LEN			6
#define DCF_DAY_START			36
#define DCF_DAY_LEN				6
#define DCF_WDAY_START		42
#define DCF_WDAY_LEN			3
#define DCF_MONTH_START		45
#define DCF_MONTH_LEN			5
#define DCF_YEAR_START		50
#define DCF_YEAR_LEN			8

// DCF flag bit offsets
#define DCF_CW_START	1
#define DCF_CW_LEN		14
#define DCF_R_BIT			15
#define DCF_A1_BIT		16
#define DCF_Z1_BIT		17
#define DCF_Z2_BIT		18
#define DCF_A2_BIT		19

// 64 bit macros
#define bitRead64(value, bit) (((value) >> (bit)) & 1ULL)
#define bitSet64(value, bit) ((value) |= (1ULL << (bit)))
#define bitClear64(value, bit) ((value) &= ~(1ULL << (bit)))
#define bitWrite64(value, bit, bitvalue) (bitvalue ? bitSet64(value, bit) : bitClear64(value, bit))

class dcfTime
{
private:

// a function to return a 'chunk' of up to 8 bits from the bitBuffer
	uint16_t getTimeValue(uint8_t _start, uint8_t _len);

// a function to return the parity of a 'chunk' of bits in the bitBuffer
	uint8_t checkParity(uint8_t _start, uint8_t _numBits, uint8_t _parityBit, uint8_t _index);

public:

dcfTime();

// the simple begin method using just the interrupt pin number
int8_t begin(int8_t _intPin);

// the full begin method using both the interrupt pin number and interrupt number
int8_t begin(int8_t _intPin, uint8_t _intNum);

// enable a pin as an LED driver with the level required to switch the LED ON
void setLedPin(int8_t _pin, uint8_t _level);

// converts a BCD Byte into decimal
uint8_t bcdToDec(uint8_t _val);

// converts decimal Byte to BCD
uint8_t decToBcd(uint8_t _val);

// the RX pulse interrupt routine
void rxPulseISR(void);

// check if the time has been decoded and is available.
// Returns the millis of the start of the minute or zero
// if the time is not available.
uint32_t timeAvailable(void);

// pins and levels
volatile uint8_t pulseInputPin, pulseCarrierOffLevel,ledPin, ledOnLevel;

// the timestamps
volatile time_t lclTimestamp, ukTimestamp, utcTimestamp;
// the Civil Warning bits
volatile uint16_t CWbits;
// various other variables
volatile bool ledOn;
volatile uint8_t intNum, carrierOff, timeProcessed, thisBit, parityError, Rbit, A1bit, A2bit, Z1bit, Z2bit;
volatile uint32_t lastPulseStart, pulseStart, startOfMinute;

volatile uint64_t bitBuffer;	// an 8 Byte Unsigned Long Long used as the DCF77 bit buffer

};// END OF dcfTime class

extern dcfTime dcf;

#endif

