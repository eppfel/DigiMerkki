
/*
  CapacitiveKeyboard.cpp - Library for CapacitiveKeyboard
  Created by Felix A. Epp
*/

#include "Arduino.h"
#include "CapacitiveKeyboard.h"

CapacitiveKeyboard::CapacitiveKeyboard(int pin1, int pin2, int pin3, int treshhold) : _button1(pin1, DEBOUNCE_TIME, treshhold), _button2(pin2, DEBOUNCE_TIME, treshhold), _button3(pin3, DEBOUNCE_TIME, treshhold)
{
	_lastButton = 0;
}

void CapacitiveKeyboard::begin(){
  _button1.begin();
  _button2.begin();
  _button3.begin();
}

void CapacitiveKeyboard::onPressed(EasyButtonBase::callback_t callback) {
  _button1.onPressed(callback);
  _button2.onPressed(callback);
  _button3.onPressed(callback);
}

/* Check for touch input */
// Needs a buffer to avoid noise
int CapacitiveKeyboard::checkTouch() {

  unsigned int buttonState = 0;
    
//   long capval = touchRead(_pin1);
//   ADCFilter.Filter(capval);
//   if (ADCFilter.Current() < _treshhold) {
//     buttonState += BTN_A;
//   }

//   capval = touchRead(_pin2);
//   ADCFilter1.Filter(capval);
//   if (ADCFilter1.Current() < _treshhold) {
//     buttonState += BTN_B;
//   }

//   capval = touchRead(_pin3);
//   ADCFilter2.Filter(capval);
//   if (ADCFilter2.Current() < _treshhold) {
//     buttonState += BTN_C;
//   }

// #ifdef DEBUG
//   if (buttonState != _lastButton) {
//     switch (buttonState) {
//       case BTN_A:
//         Serial.print("Button press: A : ");
//         Serial.println(ADCFilter.Current());
//         break;
//       case BTN_B:
//         Serial.print("Button press: B : ");
//         Serial.println(ADCFilter1.Current());
//         break;
//       case BTN_C:
//         Serial.print("Button press: C : ");
//         Serial.println(ADCFilter2.Current());
//         break;
//       case BTN_AB:
//         Serial.print("Button press: A+B : ");
//         Serial.println(ADCFilter.Current());
//         Serial.print(", ");
//         Serial.println(ADCFilter1.Current());
//         break;
//       case BTN_AC:
//         Serial.print("Button press: A+C : ");
//         Serial.print(ADCFilter.Current());
//         Serial.print(", ");
//         Serial.println(ADCFilter2.Current());
//         break;
//       case BTN_BC:
//         Serial.print("Button press: B+C : ");
//         Serial.print(ADCFilter1.Current());
//         Serial.print(", ");
//         Serial.println(ADCFilter2.Current());
//         break;
//       case BTN_ABC:
//         Serial.print("Button press: A+B+C : ");
//         Serial.print(ADCFilter.Current());
//         Serial.print(", ");
//         Serial.print(ADCFilter1.Current());
//         Serial.print(", ");
//         Serial.println(ADCFilter2.Current());
//         break;
//       default:
//         Serial.println("No button was pressed.");
//         break;
//     }
//     _lastButton = buttonState;
//   }
// #endif

  return buttonState;
}

void CapacitiveKeyboard::tick() {
  _button1.read();
  _button2.read();
  _button3.read();
}