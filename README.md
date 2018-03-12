# Arduino-DCF77-Time-Library
A simple DCF77 decoder library for the Arduino

There are a few libraries available to decode the DCF77 time signal from Mainflingen, Germany but I decided to write my own using the simplest method I could devise. The result is presented here and it works very reliably.

Points to note:

The signal we are decoding is transmitted on 77.5KHz and at this frequency the signal is very prone to interference from all sorts of devices from computers to car ignitions. One basic mistake made by those new to DCF and MSF signal reception is to place their receiver and antenna too close to the decoding MCU such as an Arduino UNO which produces enough interference to override the incoming signal.

Radio Controlled clocks often suffer from interference but, as they are usually hung on walls which are well away from interference sources, they work quite effectively. trying to construct a breadboard project with the DCF receiver and antenna close to the MCU is going to fail!

The best results are obtained by mounting both the receiver and antenna in a box which can be placed high up on a shelf or in a clear location well away from interference sources. A 3 or 4 core cable can be run from the receiver box to the MCU up to 2 metres away. The receiver antenna must be horizontal and at right angles to the transmitter.

The example sketches given here are written for the Arduino UNO but are suitable for the NANO etc. and, with a little adaption, the sketches will even work on an ESP8266. The dcfTime library uses the External interrupts only e.g. INTO (D2 on the UNO). I have tried pin interrupts but problems arise so it is wise not to change this approach. Anyway, INT0 has the highest priority!
