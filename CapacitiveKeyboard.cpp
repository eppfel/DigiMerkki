
/*
  CapacitiveKeyboard.cpp - Library for CapacitiveKeyboard
  Created by Felix A. Epp
*/

#include "Arduino.h"
#include "CapacitiveKeyboard.h"

//calibrarte button threshold
void CapacitiveKeyboard::calibrate()
{
  uint32_t leftTouch = 0, rightTouch = 0;
  for (size_t i = 0; i < CALIBRATION_SAMPLES; i++)
  {
    leftTouch += touchRead(TOUCHPIN_LEFT);
    rightTouch += touchRead(TOUCHPIN_RIGHT);
    delay(700 / CALIBRATION_SAMPLES);
  }
  leftTouch /= CALIBRATION_SAMPLES;
  rightTouch /= CALIBRATION_SAMPLES;
  _buttonLeft.setThreshold(leftTouch - SENSITIVITY_RANGE);
  _buttonRight.setThreshold(rightTouch - SENSITIVITY_RANGE);
}

void CapacitiveKeyboard::begin()
{
  _buttonState = 0;
  _buttonLeft.begin();
  _buttonRight.begin();
}

void CapacitiveKeyboard::pressed(){
  if (_buttonState > TAP_BOTH) {
    // for releasing a long press HOW?
  }
  else if (_buttonLeft.wasReleased() || _buttonRight.wasReleased())
  {
    if (_buttonLeft.wasReleased()) {
      _buttonState = _buttonState | TAP_LEFT;
    } else if (_buttonRight.wasReleased()) {
      _buttonState = _buttonState | TAP_RIGHT;
    }

    if (_buttonLeft.isReleased() && _buttonRight.isReleased()) {
      _multipressed_callback_int(_buttonState);
      _buttonState = 0;
    }
  }
}

//catch the press in this class and check which buttons was/were pressed (checking states) and return keyCode
void CapacitiveKeyboard::setBtnHandlers(CapacitiveKeyboard::callback_int_t callback_int, EasyButtonBase::callback_t callback) {
  _multipressed_callback_int = callback_int;
  _buttonLeft.onPressed(callback);
  _buttonRight.onPressed(callback);
  _buttonLeft.onPressedFor(BTNHOLDDELAY, callback);
  _buttonRight.onPressedFor(BTNHOLDDELAY, callback);
}

void CapacitiveKeyboard::tick() {
  _buttonLeft.read();
  _buttonRight.read();
}