
/*
  CapacitiveKeyboard.cpp - Library for CapacitiveKeyboard
  Created by Felix A. Epp
*/

#include "Arduino.h"
#include "CapacitiveKeyboard.h"

void CapacitiveKeyboard::begin()
{
  _buttonState = 0;
  _button1.begin();
  _button2.begin();
}

void CapacitiveKeyboard::pressed(){
  if (_buttonState > TAP_BOTH) {
    // for releasing a long press HOW?
  }
  else if (_button1.wasReleased() || _button2.wasReleased())
  {
    if (_button1.wasReleased()) {
      _buttonState = _buttonState | TAP_LEFT;
    } else if (_button2.wasReleased()) {
      _buttonState = _buttonState | TAP_RIGHT;
    }

    if (_button1.isReleased() && _button2.isReleased()) {
      _multipressed_callback_int(_buttonState);
      _buttonState = 0;
    }
  }
}

//catch the press in this class and check which buttons was/were pressed (checking states) and return keyCode
void CapacitiveKeyboard::setBtnHandlers(CapacitiveKeyboard::callback_int_t callback_int, EasyButtonBase::callback_t callback) {
  _multipressed_callback_int = callback_int;
  _button1.onPressed(callback);
  _button2.onPressed(callback);
  _button1.onPressedFor(BTNHOLDDELAY, callback);
  _button2.onPressedFor(BTNHOLDDELAY, callback);
}

void CapacitiveKeyboard::tick() {
  _button1.read();
  _button2.read();
}