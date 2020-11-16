/*
  StatusVisualiser.h - Class for handling a visualisation of a set of RGB LEDs with FastLED
  Created by Felix A. Epp
*/
#ifndef StatusVisualiser_h
#define StatusVisualiser_h

#define USE_GET_MILLISECOND_TIMER

#include "Arduino.h"
#include <FastLED.h>

#ifndef NEOPIXEL_PIN
#define NEOPIXEL_PIN 12 // Pin for controlling NeoPixel
#endif

class StatusVisualiser {
  private:
    CRGB _leds[NUM_LEDS];// include variables for addresable LEDs
    uint8_t _currentState = STATE_ANIMATION;
    uint8_t _transitionState = _currentState;
    uint8_t _currentPattern = PATTERN_OFF;
    uint8_t _maxBrightness = 64;
    uint8_t _bpm = 60;
    CRGB _animationColor = CRGB::White; 

    bool _blinkFlag;
    uint32_t _animationStart = 0;
    uint32_t _animationPhase;
    uint8_t _animationIterations;
    CRGB _blinkColor;

  public:
    static const uint8_t STATE_STATIC = 0;
    static const uint8_t STATE_ANIMATION = 1;
    static const uint8_t STATE_METER =     2;
    static const uint8_t STATE_BLINKING =  3;
    static const uint8_t PATTERN_OFF =     0;
    static const uint8_t PATTERN_CYLON =   1;
    static const uint8_t PATTERN_SPREAD = 2;
    static const uint8_t PATTERN_SUCK = 3;
    static const uint8_t PATTERN_MOVINGRAINBOW = 4;
    static const uint8_t PATTERN_RAINBOWBEAT = 5;
    static const uint8_t PATTERN_BEATFADE = 6;

    StatusVisualiser(uint32_t (*t)(), uint8_t maxBrightness);

    void show();
    void turnOff();
    void blink(uint32_t phase, uint8_t iterations, CRGB color, uint8_t transitionState = STATE_ANIMATION);
    void setMeterFromIndex(int8_t ledIndex);
    void setMeter(int8_t ledIndex = -1);
    void fillMeter(uint32_t fromT, uint32_t toT, int32_t colorCode = CRGB::White);
    void cylon(CRGB color = CRGB::White, uint8_t bpm = 60);
    void nextPattern();
};

#endif