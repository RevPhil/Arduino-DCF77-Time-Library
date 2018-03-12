#include <dcfTime.h>

// static function for the interrupt handler
dcfTime *DCF = NULL;
void dcfIntChange() 
{
	if (DCF) DCF->rxPulseISR();
}

// preset the system variables
dcfTime::dcfTime()
{
	pulseInputPin = DCF_PULSE_INPUT_PIN;
	pulseCarrierOffLevel = DCF_PULSE_CARRIER_OFF_LEVEL;
	ledPin = DCF_LED_PIN;
	ledOnLevel = DCF_LED_ON_LEVEL;
};

// simple begin method using just the interrupt pin
// returns -1 if the interrupt pin is not valid
int8_t dcfTime::begin(int8_t _intPin) {
	intNum = digitalPinToInterrupt(_intPin);
	return begin(_intPin, intNum);
}

// full begin method using the interrupt pin and interrupt number
// returns -1 if the interrupt pin is not valid
int8_t dcfTime::begin(int8_t _intPin, uint8_t _intNum) {
	if(_intNum < 0) return _intNum;
	pulseInputPin = _intPin;
	pinMode(ledPin, OUTPUT);
	digitalWrite(ledPin, !ledOnLevel);
	DCF = this; // singleton pointer for ISR
	attachInterrupt(intNum,dcfIntChange,CHANGE);
	timeProcessed = false;
	return _intNum;
}

// enable a pin as an LED driver with the level required to switch the LED ON
// the LED is turned off
void dcfTime::setLedPin(int8_t _pin, uint8_t _level) {
	ledPin = _pin;
	ledOnLevel = _level?HIGH:LOW;
}

// the interrupt ISR to process the DCF77 pulses.
// the result is the "thisBit" variable which is the DCF pulse
// length in milliseconds. The timeAvailable() method processes thisBit
// so it must be called very regularly in the sketch loop
void dcfTime::rxPulseISR(void) {
	// check the state of the interrupt pin
	if(digitalRead(pulseInputPin) == pulseCarrierOffLevel) {
		// this must be the start of a pulse
    pulseStart = millis();
		// check if this is the first second of a new minute and set startOfMinute millis
		if((pulseStart - lastPulseStart) > 1700) {
			startOfMinute = pulseStart;
		}
		else startOfMinute = false;
		ledOn = true;	// turn on the LED
    return;
		}
		else {
    // this must be the end of a pulse so calculate how long the pulse was
		thisBit = (millis() - pulseStart) + DCF_PULSE_EXTEND_TIME;
		ledOn = false;	// turn off the LED
		}
	}// END OF rxPulseISR

// this is the main routine which must be called very regularly from within the sketch loop()
// It returns true if startOfMinute and timeProcessed are true which indicates
// that the timestamps can be retrieved.
uint32_t dcfTime::timeAvailable(void) {
	static uint8_t counter = 0, timeStart = 0;
	// if startOf Minute AND timeProcessed return true
	if(startOfMinute && timeProcessed) {
		timeProcessed = false;
		return startOfMinute;
	}
	
	// turn the LED On or Off
	if(ledOn) digitalWrite(ledPin,ledOnLevel);
	else digitalWrite(ledPin,!ledOnLevel);
	
	if(thisBit >= DCF_MIN_PULSE && thisBit < DCF_MAX_PULSE) {
		// If this is the first pulse of the minute, reset everything for processing
		if(startOfMinute) {
			counter = 0;
			timeProcessed = false;
			bitBuffer = 0;
		}
		lastPulseStart = pulseStart;
		// if this is the 2nd second reset the parityError flags and timestamps
		// as the user doesn't need the data anymore
		if(counter == 1) {
			startOfMinute = false;
			parityError = 0;
			lclTimestamp = 0;
			ukTimestamp = 0;;
			utcTimestamp = 0;
		}
		// turn the pulse length ~100ms or ~200ms into a 0 or 1
		thisBit = (thisBit/100)-1;
#if DCF_DEBUG
	if(startOfMinute && counter == 0) {
		Serial.print(F("\r\n Civil Warning- Flags T Minute- P Hour-- P Day--- WDy Month Year---- P"));
		Serial.print("\r\nS");
	}
	else {
		uint8_t c = counter;
		if(c==15||c==20||c==21||c==28||c==29||c==35||c==36||c==42||c==45||c==50||c==58) Serial.print(" ");
		Serial.print(thisBit);	// output the bit value
	}
#endif
		// write the bit to the buffer
		bitWrite64(bitBuffer, counter, thisBit);
		// second 20 is the Time Start bit which MUST be 1 so we can use this as a double check later
		if(counter == DCF_TIME_START_BIT) timeStart = thisBit;
		// check for buffer overrun, clear the counter
		if(counter > 61)
		{
			counter = 0;
#if DCF_DEBUG
	Serial.println();
#endif
		}
		else	counter++;

		thisBit = false;

		// this section is only called at the 59th second of the minute. All the decoding of the
		// DCF77 bit stream takes place here! Any extra pulses such as a leap second are ignored
		// because there is no point trying to do anything with leap seconds in this application.
		if(counter == 59 && timeStart == 1) {	
#if DCF_DEBUG
	Serial.println();
#endif
			timeStart = false;
			// ckeckParity returns zero if all is well. If not, checkParity returns these values:
			// 1	= Minute parity error
			// 2	= Hour parity error
			// 4	= Date parity error
			parityError = (checkParity(21,7,28,1) || checkParity(29,6,35,2) || checkParity(36,22,58,4));
			// get the Civil Warning bits into the CWbits word
			CWbits = getTimeValue(DCF_CW_START,DCF_CW_LEN);
			
			// set the FLAG variables
			Rbit = bitRead64(bitBuffer, DCF_R_BIT);    // Backup Antenna in use
			A1bit = bitRead64(bitBuffer, DCF_A1_BIT);  // DST Announcement (for 1 hour before start of DST)
			Z1bit = bitRead64(bitBuffer, DCF_Z1_BIT);  // DST (CEST) in effect
			Z2bit = bitRead64(bitBuffer, DCF_Z2_BIT);  // CET in effect (No DST)
			A2bit = bitRead64(bitBuffer, DCF_A2_BIT);  // Leap Second Announcement (for 1 hour before change)
			
			// fill the tm structure for calculating the timestamp. We don't need the DCF weekday for this.
			// the DCF time values are BCD so we convert them to Decimal
			tmElements_t tm;
			tm.Second = 0;
			tm.Minute = bcdToDec(getTimeValue(DCF_MINUTE_START,DCF_MINUTE_LEN));	// minutes	7 bits 0 - 59
			tm.Hour = bcdToDec(getTimeValue(DCF_HOUR_START,DCF_HOUR_LEN));				// hours		6 bits 0 - 23
			tm.Day = bcdToDec(getTimeValue(DCF_DAY_START,DCF_DAY_LEN));					// date			6 bits 1 - 31
			tm.Month = bcdToDec(getTimeValue(DCF_MONTH_START,DCF_MONTH_LEN));		// month		5 bits 1 - 12
			tm.Year = bcdToDec(getTimeValue(DCF_YEAR_START,DCF_YEAR_LEN)) + 30;				// year			8 bits 0 - 99
			
			// make a timestamp of the local, UK and UTC time
			lclTimestamp = makeTime(tm);
			ukTimestamp = lclTimestamp - 3600UL;
			utcTimestamp = Z1bit?lclTimestamp - 7200UL:lclTimestamp - 3600UL;
			// set the timeProcessed flag for the start of the next minute
			if(!parityError) timeProcessed = true;
		}
	}
	return false;
} // END OF timeAvailable()


uint16_t dcfTime::getTimeValue(uint8_t _start, uint8_t _len) {
  // reads the number of bits specified from the bitBuffer into tempTimeValue
  uint16_t tempTimeValue = 0; 
	for(uint8_t x = 0;x<_len;x++) bitWrite(tempTimeValue,x,bitRead(bitBuffer,_start+x));
	return tempTimeValue; 
}//END OF getTimeValue


uint8_t dcfTime::checkParity(uint8_t _start, uint8_t _numBits, uint8_t _parityBit, uint8_t _index) {
  // gathers the required bits in the bitBuffer and calculates the EVEN parity
  uint8_t parity = 0;
  // cycle through the bitBuffer and add the bits together in parity
	for(uint8_t x = 0;x<_numBits;x++) parity += bitRead64(bitBuffer, _start+x);
  // compare the parityVal bit with bit 0 of the parity variable
  // return the _index as an error number e.g. 1, 2 or 4 
	if(bitRead64(bitBuffer, _parityBit) != (parity & 0x01)) return _index;
  return 0;
}//END OF checkParity

// Convert binary coded decimal to normal decimal numbers
uint8_t dcfTime::bcdToDec(uint8_t _val) {
  return ( (_val/16*10) + (_val%16) );
}//END OF bcdToDec

// Convert normal decimal numbers to binary coded decimal
uint8_t dcfTime::decToBcd(uint8_t _val) {
	return ( (_val/10*16) + (_val%10) );
}//END OF decToBcd

dcfTime dcf = dcfTime();