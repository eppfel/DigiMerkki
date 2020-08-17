/*
  StatusVisualiser.h - Class for handling a visualisation of a set of RGB LEDs with FastLED
  Created by Felix A. Epp
*/
#ifndef StatusVisualiser_h
#define StatusVisualiser_h

#define USE_GET_MILLISECOND_TIMER

#include "Arduino.h"
#include <FastLED.h>


#ifdef ESP8266 // Feather Huzzah ESP8266
  #define DATA_PIN       12    // Pin for controlling NeoPixel
#elif defined(ARDUINO_FEATHER_ESP32)
  #define DATA_PIN       12    // Pin for controlling NeoPixel
#else //ESP32 DEV Module
  #define DATA_PIN       26    // Pin for controlling NeoPixel
#endif

#define   NUM_LEDS        5    // Number of LEDs conrolled through FastLED
#define   BS_PERIOD       360
#define   BS_COUNT        10

uint32_t (*getMeshNodeTime)();
uint32_t get_millisecond_timer();

class StatusVisualiser {
  private:
    CRGB _leds[NUM_LEDS];// include variables for addresable LEDs
    uint8_t _currentState = STATE_IDLE;
    // uint8_t _suspendedState = NULL;
    uint8_t _maxBrightness;
    uint8_t _bpm;

    bool _blinkFlag;
    uint32_t _animationStart = 0;
    uint32_t _animationPhase;
    uint8_t _animationIterations;
    CRGB _blinkColor;

  public:
    const uint8_t STATE_IDLE =     0;
    const uint8_t STATE_METER =    1;
    const uint8_t STATE_BLINKING = 2;
    const uint8_t STATE_CYLON =    3;

    StatusVisualiser(uint32_t (*t)(), uint8_t maxBrightness);

    void show();
    void blink(uint32_t phase, uint8_t iterations, CRGB color);
    void setMeter(uint8_t ledIndex);
    void cylon(uint8_t bpm = 60);

};

#endif