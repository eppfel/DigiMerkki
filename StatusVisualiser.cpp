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
	FastLED.addLeds<NEOPIXEL, NEOPIXEL_PIN>(_leds, NUM_LEDS); // GRB ordering is assumed
	FastLED.clear();
	_maxBrightness = maxBrightness;
	getMeshNodeTime = t;
	FastLED.setBrightness(maxBrightness);
	tapTempo.setBPM(DEFAULT_BPM);
}

void StatusVisualiser::show() {
	
	_bpm = tapTempo.getBPM();

	// EVERY_N_MILLIS(500)
	// {
	// 	Serial.print("BPM: ");Serial.println(_bpm);
	// }

	if (_currentState == STATE_BLINKING)
	{
		if ((get_millisecond_timer() - _animationStart) > _animationPhase * _animationIterations) {
			_currentState = _transitionState;
			FastLED.clear();
		} else if (((get_millisecond_timer() - _animationStart) / _animationPhase) % 2 != _blinkFlag)
		{
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
		if (_currentPattern == PATTERN_CYLON)
		{
			uint8_t ledPos = beatsin8(_bpm, 0, NUM_LEDS - 1);
			_leds[ledPos] = _animationColor;
			_leds[ledPos] %= 64;
			ledPos = beatsin8(_bpm, 0, NUM_LEDS - 1, 0, 20);
			_leds[ledPos] = _animationColor;
			FastLED.setBrightness(_maxBrightness);
			FastLED.show();
			fadeToBlackBy(_leds, NUM_LEDS, 255);
		}
		else if (_currentPattern == PATTERN_SPREAD)
		{
			static const uint8_t startled = NUM_LEDS / 2;
			uint8_t spread = beatsin8(_bpm, 0, startled + 1);
			if (spread) {
				uint8_t ledmin = startled - (spread - 1);
				uint8_t lednum = spread * 2 - 1;
				fill_solid(&(_leds[ledmin]), lednum, _animationColor);
				FastLED.setBrightness(_maxBrightness);
			}
			FastLED.show();
			fadeToBlackBy(_leds, NUM_LEDS, 255);
		}
		else if (_currentPattern == PATTERN_SUCK)
		{
			static const uint8_t startled = NUM_LEDS / 2;
			uint8_t spread = beatsin8(_bpm, 0, startled + 1);
			if (spread)
			{
				uint8_t ledmin = startled - (spread - 1);
				uint8_t lednum = spread * 2 - 1;
				fill_solid(&(_leds[ledmin]), lednum, _animationColor);
				FastLED.setBrightness(_maxBrightness);
			}
			FastLED.show();
			fadeToBlackBy(_leds, NUM_LEDS, 255);
		}
		else if (_currentPattern == PATTERN_MOVINGRAINBOW)
		{	
			uint8_t phase = (((get_millisecond_timer() - _animationStart) % _animationPhase) / (_animationPhase / 255));
			fill_rainbow(_leds, NUM_LEDS, phase, 17); // Use FastLED's fill_rainbow routine.
			FastLED.show();
		}
		else if (_currentPattern == PATTERN_RAINBOWBEAT)
		{	
			uint8_t beatA = beatsin8(_bpm / 2, 0, 255); // Starting hue
			fill_rainbow(_leds, NUM_LEDS, beatA, 12); // Use FastLED's fill_rainbow routine.
			FastLED.show();
		}
		else if (_currentPattern == PATTERN_BEATFADE)
		{
			if ((get_millisecond_timer() - _animationStart) > _animationPhase)
			{
				fill_solid(&(_leds[0]), NUM_LEDS, _animationColor);
				_animationStart += _animationPhase;
			}
			FastLED.show();
			fadeToBlackBy(_leds, NUM_LEDS, 16);
		}
		else
		{
			FastLED.show();
		}
	}
	else if (_currentState == STATE_METER) {
		uint8_t meterIndex = (get_millisecond_timer() - _animationStart) / (_animationPhase / NUM_LEDS);
		setMeterFromIndex(meterIndex);
		FastLED.show();
	}
	else
	{
		FastLED.show();
	}

}

void StatusVisualiser::turnOff()
{
	_currentState = STATE_ANIMATION;
	_currentPattern = PATTERN_OFF;
	FastLED.clear();
	FastLED.show();
}

void StatusVisualiser::blink(uint32_t phase, uint8_t iterations, CRGB color, uint8_t transitionState)
{
	_animationStart = get_millisecond_timer();
	_animationPhase = (uint32_t) ((float) phase / 2);
	_animationIterations = iterations * 2;
	_blinkColor = color;
	_transitionState = transitionState;
	_currentState = STATE_BLINKING;
	_blinkFlag = true;
}

//Todo: Make function bidirectional (fill LEDs from back to front based on a flag)
void StatusVisualiser::setMeterFromIndex(int8_t ledIndex) {
	for (int8_t i = 0; i < NUM_LEDS; ++i)
	{
		if (ledIndex < i)
		{
			_leds[i] = CRGB::Black;
		} else {
			_leds[i] = CRGB::HotPink;
		}
	}
}

void StatusVisualiser::setMeter(int8_t ledIndex) {
	_currentState = STATE_STATIC;
	setMeterFromIndex(ledIndex);
}

void StatusVisualiser::fillMeter(uint32_t fromT, uint32_t duration, int32_t colorCode) {
	_currentState = STATE_METER;
	_animationStart = fromT;
	_animationPhase = duration;
}

void StatusVisualiser::cylon(CRGB color, uint8_t bpm)
{
	_animationColor = color;
	_currentState = STATE_ANIMATION;
	_currentPattern = PATTERN_SPREAD;
}

void StatusVisualiser::nextPattern()
{
	_currentPattern++;
	if (_currentPattern > PATTERN_BEATFADE) {
		_currentPattern = PATTERN_OFF;
	}

	if (_currentPattern == PATTERN_BEATFADE)
	{
		_animationStart = get_millisecond_timer();
		_animationPhase = 60000 / _bpm;
	}
	else if (_currentPattern == PATTERN_MOVINGRAINBOW)
	{
		_animationStart = get_millisecond_timer();
		_animationPhase = 4000;
	}
}