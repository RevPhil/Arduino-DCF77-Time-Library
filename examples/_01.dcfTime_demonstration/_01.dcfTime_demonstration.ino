/*  A simple demonstration of the dcfTime library and its capabilities
    enableBitOutput() MUST be set to true for the correct serial output.
    The default setting is false.

    The sketch will print each DCF pulse as it is received and when the
    minute's data has been decoded, an output of the time and flag values.

    The printed DCF bit values match this pattern when received correctly:

     Civil-Warning  State   Minutes   Hours    Date   Day Month Year
    000000000011111 RAZZA T 2222222 P 233333 P 333344 444 44444 55555555 P
    012345678901234 -1122 S 1234567 1 901234 2 678901 234 56789 01234567 3
    ----------------------------------------------------------------------
    Sxxxxxxxxxxxxxx xxxxx 1 xxxxxxx x xxxxxx x xxxxxx xxx xxxxx xxxxxxxx x

     S = Start of Minute, always 0
    TS = Time Start, always 1
    P1 = Parity bit 1 which will be even
    P2 = Parity bit 2 which will be even
    P3 = Parity bit 3 which will be even

    All time values are LSb first e.g. 0x17 wil be printed as 11101000.

    IMPORTANT! Most DCF77 receivers are 3.3 Volt devices despite many having 5 Volt Vcc supply
    capability. ALWAYS use these devices at Vcc 3.3 Volt and add a 10uF and 100nF capacitor
    at the Vcc/ GND pins of the receiver. if you wish to control the PON input of the receiver
    it MUST be pulled LOW on most receivers. If the HOST device e.g. Arduino UNO is 5V
    logic or TTL, the PON pin of the DCF77 receiver should be pulled LOW using a diode thus:

    Arduino Pin --|<|-- DCF77 receiver PON input

    A 10K pullup resistor should also be added between the PON pin at the receiver and Vcc 3.3V.
    If the HOST device is a 3.3V device no diode or 10K pullup resistor is needed.
		
		(C) Copyright 2017 Phil Morris <www.lydiard.plus.com>
		
		You are free to use this software for non-commercial purposes as long as this text
		remains intact.
*/

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#include <dcfTime.h>

#ifdef ESP8266
#define INT_PIN 12 // the ESP8266 EXTERNAL interrupt pin
#define LED_PIN 2  // the LED pin to indicate signal pulses (optional)
#define LED_ON_LEVEL LOW // the OUTPUT level to switch theLED ON
#define PON_CONTROL_PIN -1 // the digital pin to control the PON input of the RX
#define PON_ON_LEVEL LOW  // the PON pin level which switches the RX ON
#else
#define INT_PIN 2 // the AVR EXTERNAL interrupt pin
#define INT_NUM 0 // the AVR EXTERNAL interrupt number
#define LED_PIN 13  // the LED pin to indicate signal pulses (optional)
#define LED_ON_LEVEL HIGH // the OUTPUT level to switch theLED ON
#define PON_CONTROL_PIN 4 // the digital pin to control the PON input of the RX
#define PON_ON_LEVEL LOW  // the PON pin level which switches the RX ON
#endif

void setup()
{
  #ifdef ESP8266
  // disconnect any current WiFi setup/connection
  WiFi.disconnect();
  #endif
  
  Serial.begin(57600);

  // optional control and LED pins
  dcf.setLedPin(LED_PIN, LED_ON_LEVEL);
  //dcf.setPonPin(PON_CONTROL_PIN, PON_ON_LEVEL);
  //dcf.ponOn(true);

  // start the dcfTime library
  dcf.begin(INT_PIN);
  // allow received bit values to be printed on the Serial port
  //dcf.enableBitOutput(true);
  Serial.println("Waiting for DCF Sync....");
}//END OF SETUP

void loop()
{
  // check the parity flag (optional function)
  if (dcf.parityError) {
    // we must reset the parityError flag to prevent re-triggering
    dcf.parityError = false;
    Serial.print(F("             Parity Error: "));
    Serial.println(dcf.parityError);
  }

  // check the timeAvailable flag and process the dcfTime library data
  // timeAvailable is set at the start of the first second of the new minute
  if (dcf.timeAvailable())
  {
    // print out the timestamps
    Serial.print(F("\r\n          DCF77 Timestamp: "));
    Serial.println(dcf.lclTimestamp);
    Serial.print(F("             UK Timestamp: "));
    Serial.println(dcf.ukTimestamp);
    Serial.print(F("            UTC Timestamp: "));
    Serial.println(dcf.utcTimestamp);
    
    // process the timeStamps and extract the data
    Serial.println(F(" // TimeLib Weekdays are Sun = 1 to Sat = 7"));

    // print out the DCF time/date
    Serial.print(F("          Local Time/Date: "));
    printTime(dcf.lclTimestamp);

    // print out the UK time and date
    Serial.print(F("             UK Time/Date: "));
    printTime(dcf.ukTimestamp);

    // print out the UTC time and date
    Serial.print(F("            UTC Time/Date: "));
    printTime(dcf.utcTimestamp);

    // print the Civil Warning bits
    Serial.print(F("       Civil Warning Bits: "));
    for (uint8_t x = 0; x < 14; x++) Serial.print(bitRead(dcf.CWbits, x));
    // print out the flags
    Serial.print(F("\r\nUsing Backup Antenna (R-): "));
    Serial.println(dcf.Rbit ? "Yes" : "No");
    Serial.print(F("CEST Change Imminent (A1): "));
    Serial.println(dcf.A1bit ? "Yes" : "No");
    Serial.print(F("                CEST (Z1): "));
    Serial.println(dcf.Z1bit ? "Yes" : "No");
    Serial.print(F("                 CET (Z2): "));
    Serial.println(dcf.Z2bit ? "Yes" : "No");
    Serial.print(F("Leap Second Imminent (A2): "));
    Serial.println(dcf.A2bit ? "Yes" : "No");
  }

}//END OF LOOP

void printTime(time_t _time)
{
  tmElements_t tm;
  char charBuffer[24];
  breakTime(_time, tm);
  // print the time and date
  sprintf(charBuffer, "%02u:%02u:%02u %u %02u-%02u-20%02u", tm.Hour, tm.Minute, tm.Second, tm.Wday, tm.Day, tm.Month, tm.Year - 30);
  Serial.println(charBuffer);
}

