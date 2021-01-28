/*
  CapacitiveKeyboard.h - Library for handling three capacitive touch sensors as key input.
  Created by Felix A. Epp
*/
#ifndef CapacitiveKeyboard_h
#define CapacitiveKeyboard_h

#include "Arduino.h"
#include <EasyButtonTouch.h>

#define NOTAP 0
#define TAP_LEFT 1
#define TAP_RIGHT 2
#define TAP_BOTH 3
#define HOLD_LEFT 4
#define HOLD_RIGHT 5
#define HOLD_BOTH 6
#define RELEASE_LEFT 7
#define RELEASE_RIGHT 8
#define RELEASE_BOTH 9

class CapacitiveKeyboard
{
  public:
    using callback_int_t = void (*)(uint8_t);

    CapacitiveKeyboard(int pin1, int pin2, int thresholdLeft, int thresholdRight, uint32_t debounce_time = 35) : _buttonLeft(pin1, debounce_time, thresholdLeft), _buttonRight(pin2, debounce_time, thresholdRight)
    {
    }
    EasyButtonTouch _buttonLeft;
    EasyButtonTouch _buttonRight;
    void calibrate();
    void begin();
    void tick();
    void pressed();
    void pressedFor();
    void setBtnHandlers(CapacitiveKeyboard::callback_int_t callback_int, EasyButtonBase::callback_t callback);
    // void onPressedFor(CapacitiveKeyboard::callback_int_t callback_int, EasyButtonBase::callback_t callback);

  protected:
    uint8_t _buttonState;
    CapacitiveKeyboard::callback_int_t _multipressed_callback_int;
};

#endif