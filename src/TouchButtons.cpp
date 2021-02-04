
/*
  TouchButtons.cpp - Library for TouchButtons
  Created by Felix A. Epp
*/

#include "Arduino.h"
#include "TouchButtons.h"

//calibrate button threshold
void TouchButtons::calibrate()
{
  uint32_t leftTouch = 0, rightTouch = 0;
  for (size_t i = 0; i < CALIBRATION_SAMPLES; i++)
  {
    leftTouch += touchRead(TOUCHPIN_LEFT);
    rightTouch += touchRead(TOUCHPIN_RIGHT);
    delay(CALIBRATION_TIME / CALIBRATION_SAMPLES);
  }
  leftTouch /= CALIBRATION_SAMPLES;
  rightTouch /= CALIBRATION_SAMPLES;
  leftTouch = leftTouch - SENSITIVITY_RANGE;
  rightTouch = rightTouch - SENSITIVITY_RANGE;
  _buttonLeft.setThreshold(leftTouch);
  _buttonRight.setThreshold(rightTouch);
}

void TouchButtons::begin()
{
  _buttonState = 0;
  _buttonLeft.begin();
  _buttonRight.begin();
}

void TouchButtons::pressedFor()
{
  if (_buttonLeft.isPressed() && _buttonRight.isPressed())
  {
    _buttonState = HOLD_BOTH;
  }
  else if (_buttonLeft.isPressed())
  {
    _buttonState = HOLD_LEFT;
  }
  else //if (_buttonRight.isPressed())
  {
      _buttonState = HOLD_RIGHT;
  }
  _multipressed_callback_int((InputType)_buttonState);
  _buttonState = NO_TAP;
}

void TouchButtons::pressed()
{
  if (_buttonLeft.wasReleased()) {
    _buttonState = _buttonState | TAP_LEFT;
  } else if (_buttonRight.wasReleased()) {
    _buttonState = _buttonState | TAP_RIGHT;
  }
  if (_buttonLeft.isReleased() && _buttonRight.isReleased())
  {
    _multipressed_callback_int((InputType)_buttonState);
    _buttonState = NO_TAP;
  }
}

//catch the press in this class and check which buttons was/were pressed (checking states) and return keyCode
void TouchButtons::setBtnHandlers(TouchButtons::callback_int_t callback_int, EasyButtonBase::callback_t callback, EasyButtonBase::callback_t callbackFor)
{
  _multipressed_callback_int = callback_int;
  _buttonLeft.onPressed(callback);
  _buttonRight.onPressed(callback);
  _buttonLeft.onPressedFor(BTNHOLDDELAY, callbackFor);
  _buttonRight.onPressedFor(BTNHOLDDELAY, callbackFor);
}

void TouchButtons::tick()
{
  _buttonLeft.read();
  _buttonRight.read();
}