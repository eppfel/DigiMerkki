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

#define DEBOUNCE_TIME 35

class CapacitiveKeyboard
{
  public:
    typedef void (*callback_int_t)(uint8_t);

    CapacitiveKeyboard(int pin1, int pin2, int threshold) : _button1(pin1, DEBOUNCE_TIME, threshold), _button2(pin2, DEBOUNCE_TIME, threshold)
    {
    }
    void begin();
    void tick();
    void pressed();
    void onPressed(CapacitiveKeyboard::callback_int_t callback_int, EasyButtonBase::callback_t callback);
  protected:
    uint8_t _buttonState;
    CapacitiveKeyboard::callback_int_t _multipressed_callback_int;
    EasyButtonTouch _button1;
    EasyButtonTouch _button2;
};

#endif