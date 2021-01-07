/*
  StatusVisualiser.h - Class for handling a visualisation of a set of RGB LEDs with FastLED
  Created by Felix A. Epp
*/
#ifndef StatusVisualiser_h
#define StatusVisualiser_h

#define USE_GET_MILLISECOND_TIMER

#include "Arduino.h"
#include <FastLED.h>
#include <ArduinoTapTempo.h>

#ifndef NEOPIXEL_PIN
#define NEOPIXEL_PIN 12 // Pin for controlling NeoPixel
#endif

enum proximityStatus_t
{
  PROXIMITY_ALONE,
  PROXIMITY_NEARBY,
  PROXIMITY_GROUP
};

class StatusVisualiser
{
private:
  CRGB _leds[NUM_LEDS]; // include variables for addresable LEDs
  uint8_t _currentState = STATE_ANIMATION;
  uint8_t _transitionState = _currentState;
  uint8_t _currentPattern = PATTERN_OFF;
  uint8_t _maxPattern = PATTERN_SPREAD;
  uint8_t _maxBrightness = 64;
  float _bpm = 60;
  CRGB _animationColor = CRGB::White;

  bool _blinkFlag;
  uint32_t _animationStart = 0;
  uint32_t _animationPhase;
  uint8_t _animationIterations;
  CRGB _blinkColor;

  proximityStatus_t _proximity = PROXIMITY_ALONE;

public:
  static const uint8_t STATE_STATIC = 0;
  static const uint8_t STATE_ANIMATION = 1;
  static const uint8_t STATE_METER = 2;
  static const uint8_t STATE_BLINKING = 3;
  static const uint8_t PATTERN_OFF = 0;
  static const uint8_t PATTERN_CYLON = 1;
  static const uint8_t PATTERN_SPREAD = 2;
  static const uint8_t PATTERN_BEATFADE = 3;
  static const uint8_t PATTERN_MOVINGRAINBOW = 4;
  static const uint8_t PATTERN_RAINBOWBEAT = 5;

  StatusVisualiser(uint32_t (*t)(), uint8_t maxBrightness);
  ArduinoTapTempo tapTempo;

  void show();
  void turnOff();
  void blink(uint32_t phase, uint8_t iterations, CRGB color, uint8_t transitionState = STATE_ANIMATION);
  void setMeterFromIndex(int8_t ledIndex);
  void setMeter(int8_t ledIndex = -1);
  void fillMeter(uint32_t fromT, uint32_t toT, int32_t colorCode = CRGB::White);
  void cylon(CRGB color = CRGB::White, uint8_t bpm = 60);
  void nextPattern();
  void startPattern(uint8_t pattern);
    void setProximityStatus(proximityStatus_t proxStat);
};

#endif