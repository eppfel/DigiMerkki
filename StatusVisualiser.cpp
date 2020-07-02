/*
  StatusVisualiser.cpp - Class for handling a visualisation of a set of RGB LEDs with FastLED
  Created by Felix A. Epp
*/

#include "Arduino.h"
#include "StatusVisualiser.h"


StatusVisualiser::StatusVisualiser(){
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(_leds, NUM_LEDS);  // GRB ordering is assumed  
  FastLED.clear();
  FastLED.setBrightness(64);
}


void StatusVisualiser::show() {

	if (_currentState == STATE_BLINKING)
	{
		if ((millis() - _animationStart) > _animationPhase * _animationIterations) {
			_currentState = STATE_IDLE;
			FastLED.clear();
		} else if (((millis() - _animationStart) / _animationPhase) % 2 != _blinkFlag)
		{
			// Serial.println()
			_blinkFlag = !_blinkFlag;
			if (_blinkFlag)
			{
				fill_solid( &(_leds[0]), NUM_LEDS, CRGB( 255, 68, 221) );
			} else {
				fill_solid( &(_leds[0]), NUM_LEDS, CRGB::Black );
			}
			//    for (int i = 0; i < NUM_LEDS; ++i)
	    // {
	    //   if (i % 2 == bsFlag) {
	    //     leds[i].setHue(224);
	    //   } else {
	    //     leds[i].setHue(128);
	    //   }
	    // }
		}
  	FastLED.show();
	}

}


void StatusVisualiser::blink(uint32_t phase, uint8_t iterations) {
	_animationStart = millis();
	_animationPhase = (uint32_t) ((float) phase / 2);
	_animationIterations = iterations * 2;

	_currentState = STATE_BLINKING;
	_blinkFlag = true;
}

void StatusVisualiser::setMeter(uint8_t ledIndex) {
	_currentState = STATE_METER;

	for (int i = 0; i < NUM_LEDS; ++i)
	{
		if (ledIndex < i)
		{
			_leds[i] = CRGB::Black;
		} else {
			_leds[i] = CRGB::HotPink;
		}
	}

  FastLED.show();
}

