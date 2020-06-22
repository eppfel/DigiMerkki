/*
  CapacitiveKeyboard.h - Library for handling three capacitive touch sensors as key input.
  Created by Felix A. Epp
*/
#ifndef CapacitiveKeyboard_h
#define CapacitiveKeyboard_h

#include "Arduino.h"
#include <Filter.h>
#ifndef ESP32 // e.g. Feather Huzzah ESP8266
  #include <CapacitiveSensor.h>
#endif

#define BTN_A   1
#define BTN_B   2
#define BTN_C   4
#define BTN_AB  3
#define BTN_AC  5
#define BTN_BC  6
#define BTN_ABC 7

class CapacitiveKeyboard
{
  public:
    CapacitiveKeyboard(int pin1, int pin2, int pin3, int treshhold, int sendPin);
    int checkTouch();
  private:
    unsigned int _pin1;
    unsigned int _pin2;
    unsigned int _pin3;
    unsigned int _treshhold;
#ifndef ESP32 // e.g. ESP8266
    CapacitiveSensor _cap1;
    CapacitiveSensor _cap2;
    CapacitiveSensor _cap3;
#endif
	ExponentialFilter<long> ADCFilter;
	ExponentialFilter<long> ADCFilter1;
	ExponentialFilter<long> ADCFilter2;
	unsigned long _lastTouch;
	unsigned int _lastButton;
};

#endif