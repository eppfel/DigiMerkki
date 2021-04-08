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
public:
  enum visualiserState_t
  {
    STATE_STATIC,
    STATE_ANIMATION,
    STATE_METER,
    STATE_BLINKING
  };

  enum visualiserPattern_t
  {
    PATTERN_OFF,
    PATTERN_CYLON,
    PATTERN_SPREAD,
    PATTERN_GLITTER,
    PATTERN_STROBE,
    PATTERN_SECONDS,
    PATTERN_MOVINGRAINBOW,
    PATTERN_RAINBOWBEAT
  };

  StatusVisualiser(uint32_t (*t)(), uint8_t maxBrightness);

  void show();
  void turnOff();
  void setDefaultColor(uint32_t color);
  void fillAll();
  void fillAll(uint32_t color);
  void blink(uint32_t phase, uint8_t iterations, uint32_t color, visualiserState_t transitionState = STATE_ANIMATION);
  void setMeterFromIndex(int8_t ledIndex);
  void setMeter(int8_t ledIndex = -1);
  void fillMeter(uint32_t fromT, uint32_t toT);
  void fillMeter(uint32_t fromT, uint32_t toT, uint32_t color);
  void cylon(uint32_t beatLength = 500);
  void cylon(uint32_t color = 0, uint32_t beatLength = 500);
  void nextPattern();
  void startPattern();
  void startPattern(visualiserPattern_t pattern);
  void setProximityStatus(proximityStatus_t proxStat);
  void updateBeat(bool pressed = false);
  unsigned long getBeatLength();
  void setBeatLength(unsigned long beatLengthMS);

private:
  ArduinoTapTempo tapTempo;
  CRGB _leds[NUM_LEDS]; // include variables for addresable LEDs
  visualiserState_t _currentState = STATE_ANIMATION;
  visualiserState_t _transitionState = _currentState;
  visualiserPattern_t _maxPattern = PATTERN_SPREAD;
  proximityStatus_t _proximity = PROXIMITY_ALONE;

  uint8_t _maxBrightness = 64;
  float _bpm = 60;

  uint32_t _defaultColor = CRGB::White;
  uint32_t _animationColor = _defaultColor;
  uint32_t _animationStart = 0;
  uint32_t _animationPhase;
  uint8_t _animationIterations;
  uint32_t _blinkColor;

  bool _blinkFlag;
};

#endif