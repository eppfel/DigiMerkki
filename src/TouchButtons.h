/*
  TouchButtons.h - Library for handling three capacitive touch sensors as key input.
  Created by Felix A. Epp
*/
#ifndef TouchButtons_h
#define TouchButtons_h

#include "Arduino.h"
#include <EasyButtonTouch.h>

class TouchButtons
{
  public:
    enum InputType
    {
      NO_TAP = 0,
      TAP_LEFT = 1,
      TAP_RIGHT = 2,
      TAP_BOTH = 3,
      HOLD_LEFT = 4,
      HOLD_RIGHT = 5,
      HOLD_BOTH = 6
    };
    using callback_int_t = void(*)(InputType);

    TouchButtons(int pin1, int pin2, int thresholdLeft, int thresholdRight, uint32_t debounce_time = 35) : _buttonLeft(pin1, debounce_time, thresholdLeft), _buttonRight(pin2, debounce_time, thresholdRight)
    {
    }
    EasyButtonTouch _buttonLeft;
    EasyButtonTouch _buttonRight;
    void calibrate();
    void begin();
    void tick();
    void pressed();
    void pressedFor();
    void setBtnHandlers(TouchButtons::callback_int_t callback_int, EasyButtonBase::callback_t callback, EasyButtonBase::callback_t callbackFor);

  protected:
    uint8_t _buttonState;
    TouchButtons::callback_int_t _multipressed_callback_int;
    bool _calibrating;
    void _addMeasurement();
};

#endif