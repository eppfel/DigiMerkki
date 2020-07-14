/*
  CapacitiveKeyboard.h - Library for handling three capacitive touch sensors as key input.
  Created by Felix A. Epp
*/
#ifndef CapacitiveKeyboard_h
#define CapacitiveKeyboard_h

#include "Arduino.h"
#include <EasyButtonTouch.h>

#define BTN_0   0
#define BTN_A   1
#define BTN_B   2
#define BTN_C   4
#define BTN_AB  3
#define BTN_AC  5
#define BTN_BC  6
#define BTN_ABC 7

#define DEBOUNCE_TIME 35

class CapacitiveKeyboard
{
  public:
    typedef void (*callback_int_t)(uint8_t);

    CapacitiveKeyboard(int pin1, int pin2, int pin3, int threshold) : _button1(pin1, DEBOUNCE_TIME, threshold), _button2(pin2, DEBOUNCE_TIME, threshold), _button3(pin3, DEBOUNCE_TIME, threshold)
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
    EasyButtonTouch _button3;
};

#endif