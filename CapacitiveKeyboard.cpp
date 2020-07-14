
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
  _button3.begin();
}

void CapacitiveKeyboard::pressed(){

  if (_button1.wasReleased()) {
    _buttonState = _buttonState | BTN_A;
  } else if (_button2.wasReleased()) {
    _buttonState = _buttonState | BTN_B;
  } else if (_button3.wasReleased()) {
    _buttonState = _buttonState | BTN_C;
  }

  if (_button1.isReleased() && _button2.isReleased() && _button3.isReleased()) {
    _multipressed_callback_int(_buttonState);
    _buttonState = 0;
  }
}

//catch the press in this class and check which buttons was/were pressed (checking states) and return keyCode
void CapacitiveKeyboard::onPressed(CapacitiveKeyboard::callback_int_t callback_int, EasyButtonBase::callback_t callback) {
  _multipressed_callback_int = callback_int;
  _button1.onPressed(callback);
  _button2.onPressed(callback);
  _button3.onPressed(callback);
}

void CapacitiveKeyboard::tick() {
  _button1.read();
  _button2.read();
  _button3.read();
}