
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

//catch the press in this class and check which buttons was/were pressed (checking states) and return keyCode
void CapacitiveKeyboard::onPressed(CapacitiveKeyboard::callback_int_t callback_int, EasyButtonBase::callback_t callback) {
  _multipressed_callback_int = callback_int;
  _button1.onPressed(callback);
  _button2.onPressed(callback);
}

void CapacitiveKeyboard::tick() {
  _button1.read();
  _button2.read();
}