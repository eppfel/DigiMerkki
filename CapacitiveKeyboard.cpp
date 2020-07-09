
/*
  CapacitiveKeyboard.cpp - Library for CapacitiveKeyboard
  Created by Felix A. Epp
*/

#include "Arduino.h"
#include "CapacitiveKeyboard.h"

CapacitiveKeyboard::CapacitiveKeyboard(int pin1, int pin2, int pin3, int treshhold, int sendPin):ADCFilter(5, treshhold), ADCFilter1(5, treshhold), ADCFilter2(5, treshhold)
{
	_lastButton = 0;
	_pin1 = pin1;
	_pin2 = pin2;
	_pin3 = pin3;
  _treshhold = treshhold;
#ifndef ESP32 // e.g. ESP8266
  _cap1 = CapacitiveSensor(sendPin,pin1); // 470k resistor between pins 4 & 2, pin 2 is sensor pin, add a wire and or foil if desired
  _cap2 = CapacitiveSensor(sendPin,pin2); // 470k resistor between pins 4 & 2, pin 2 is sensor pin, add a wire and or foil if desired
  _cap3 = CapacitiveSensor(sendPin,pin3); // 470k resistor between pins 4 & 2, pin 2 is sensor pin, add a wire and or foil if desired
  _cap1.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  _cap2.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  _cap3.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
#endif
}

/* Check for touch input */
// Needs a buffer to avoid noise
int CapacitiveKeyboard::checkTouch() {
  unsigned int buttonState = 0;

#ifdef ESP32
    
  long capval = touchRead(_pin1);
  ADCFilter.Filter(capval);
  if (ADCFilter.Current() < _treshhold) {
    buttonState += BTN_A;
  }

  capval = touchRead(_pin2);
  ADCFilter1.Filter(capval);
  if (ADCFilter1.Current() < _treshhold) {
    buttonState += BTN_B;
  }

  capval = touchRead(_pin3);
  ADCFilter2.Filter(capval);
  if (ADCFilter2.Current() < _treshhold) {
    buttonState += BTN_C;
  }
#else
  long capval = cap.capacitiveSensor(30);
  ADCFilter.Filter(capval);
  if (ADCFilter.Current() > _treshhold) {  
    buttonState += 1;
  }
    
#endif

#ifdef DEBUG
  if (buttonState != _lastButton) {
    switch (buttonState) {
      case BTN_A:
        Serial.print("Button press: A : ");
        Serial.println(ADCFilter.Current());
        break;
      case BTN_B:
        Serial.print("Button press: B : ");
        Serial.println(ADCFilter1.Current());
        break;
      case BTN_C:
        Serial.print("Button press: C : ");
        Serial.println(ADCFilter2.Current());
        break;
      case BTN_AB:
        Serial.print("Button press: A+B : ");
        Serial.println(ADCFilter.Current());
        Serial.print(", ");
        Serial.println(ADCFilter1.Current());
        break;
      case BTN_AC:
        Serial.print("Button press: A+C : ");
        Serial.print(ADCFilter.Current());
        Serial.print(", ");
        Serial.println(ADCFilter2.Current());
        break;
      case BTN_BC:
        Serial.print("Button press: B+C : ");
        Serial.print(ADCFilter1.Current());
        Serial.print(", ");
        Serial.println(ADCFilter2.Current());
        break;
      case BTN_ABC:
        Serial.print("Button press: A+B+C : ");
        Serial.print(ADCFilter.Current());
        Serial.print(", ");
        Serial.print(ADCFilter1.Current());
        Serial.print(", ");
        Serial.println(ADCFilter2.Current());
        break;
      default:
        Serial.println("No button was pressed.");
        break;
    }
    _lastButton = buttonState;
  }
#endif

  return buttonState;
}