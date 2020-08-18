/*
  StatusVisualiser.cpp - Class for handling a visualisation of a set of RGB LEDs with FastLED
  Created by Felix A. Epp
*/

#include "Arduino.h"
#include "StatusVisualiser.h"

uint32_t (*getMeshNodeTime)();

// @Override This function is called by FastLED inside lib8tion.h.Requests it to use mesg.getNodeTime instead of internal millis() timer.
// Makes every pattern on each node synced!! So awesome!
uint32_t get_millisecond_timer()
{
	return getMeshNodeTime();
}

StatusVisualiser::StatusVisualiser(uint32_t (*t)(), uint8_t maxBrightness = 64)
{
	FastLED.addLeds<NEOPIXEL, DATA_PIN>(_leds, NUM_LEDS); // GRB ordering is assumed
	FastLED.clear();
	_maxBrightness = maxBrightness;
	getMeshNodeTime = t;
	FastLED.setBrightness(maxBrightness);
}

void StatusVisualiser::show() {

	if (_currentState == STATE_BLINKING)
	{
		if ((get_millisecond_timer() - _animationStart) > _animationPhase * _animationIterations) {
			_currentState = STATE_ANIMATION;
			FastLED.clear();
		} else if (((get_millisecond_timer() - _animationStart) / _animationPhase) % 2 != _blinkFlag)
		{
			// Serial.println()
			_blinkFlag = !_blinkFlag;
			if (_blinkFlag)
			{
				fill_solid( &(_leds[0]), NUM_LEDS, _blinkColor);
			} else {
				FastLED.clear();
			}
		}
		FastLED.show();
	} else if (_currentState == STATE_ANIMATION)
	{
		if (_currentPattern == PATTERN_CYLON) {
				uint8_t ledPos = beatsin8(_bpm, 0, NUM_LEDS - 1);
				_leds[ledPos] = CRGB(85, 0, 0);
				uint8_t ledPos2 = beatsin8(_bpm, 0, NUM_LEDS - 1, 0, 20);
				_leds[ledPos2] = CRGB::Red;
				FastLED.setBrightness(_maxBrightness);
				FastLED.show();
				fadeToBlackBy(_leds, NUM_LEDS, 255);
		} else {
			FastLED.show();
		}
	} else {
		FastLED.show();
	}

}


void StatusVisualiser::blink(uint32_t phase, uint8_t iterations, CRGB color) {
	_animationStart = get_millisecond_timer();
	_animationPhase = (uint32_t) ((float) phase / 2);
	_animationIterations = iterations * 2;
	_blinkColor = color;

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

void StatusVisualiser::cylon(uint8_t bpm)
{
	_bpm = bpm;
	_currentState = STATE_ANIMATION;
	_currentPattern = PATTERN_CYLON;
}