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

    CapacitiveKeyboard(int pin1, int pin2, int pin3, int treshhold);
    int checkTouch();
    void begin();
    void tick();
    void onPressed(EasyButtonBase::callback_t callback);
    protected:
      EasyButtonBase::callback_t _pressed_callback; // Callback function for pressed events.
      EasyButtonTouch _button1;
      EasyButtonTouch _button2;
      EasyButtonTouch _button3;
      unsigned int _lastButton;
    };

#endif