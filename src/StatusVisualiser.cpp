/*
  StatusVisualiser.cpp - Class for handling a visualisation of a set of RGB LEDs with FastLED
  Created by Felix A. Epp
*/

#include "Arduino.h"
#include "StatusVisualiser.h"
#include "time.h"
#include "sys/time.h"

RTC_DATA_ATTR StatusVisualiser::visualiserPattern_t _currentPattern = StatusVisualiser::PATTERN_OFF;
RTC_DATA_ATTR unsigned long _beatLenghtMS = DEFAULT_BPM;

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
	tapTempo.setBPM(_beatLenghtMS);
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
		else if (_currentPattern == PATTERN_MOVINGRAINBOW)
		{	
			fill_rainbow(_leds, NUM_LEDS, (uint8_t)(beat8(_bpm)), 85/NUM_LEDS); // Use FastLED's fill_rainbow routine.
			FastLED.show();
		}
		else if (_currentPattern == PATTERN_RAINBOWBEAT)
		{	
			uint8_t beatA = beatsin8(_bpm / 2, 0, 255); // Starting hue
			fill_rainbow(_leds, NUM_LEDS, beatA, 12); // Use FastLED's fill_rainbow routine.
			FastLED.show();
		}
		else if (_currentPattern == PATTERN_STROBE)
		{
			if (beat8(_bpm) > 250)
			{
				fill_solid(_leds, NUM_LEDS, _animationColor); // yaw for color
			}
			else
			{
				fadeToBlackBy(_leds, NUM_LEDS, 16);
			}
			FastLED.setBrightness(_maxBrightness);
			FastLED.show();
		}
		else if (_currentPattern == PATTERN_GLITTER) {
			fill_solid(_leds, NUM_LEDS, CRGB::Black);
			if (random8() < 10)
			{
				_leds[random16(NUM_LEDS)] += _animationColor;
			}
			FastLED.setBrightness(_maxBrightness);
			FastLED.show();
		}
		else if (_currentPattern == PATTERN_SECONDS)
		{
			time_t now;
			time(&now);
			fill_solid(_leds, NUM_LEDS, CRGB::Black);
			fill_solid(_leds, (now % NUM_LEDS)+1, _animationColor);
			FastLED.setBrightness(_maxBrightness);
			FastLED.show();
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
	_currentState = STATE_STATIC;
	FastLED.clear();
	FastLED.show();
}

void StatusVisualiser::setDefaultColor(uint32_t color)
{
	_defaultColor = color;
}

void StatusVisualiser::fillAll()
{
	fillAll(_defaultColor);
}

void StatusVisualiser::fillAll(uint32_t color)
{
	_currentState = STATE_STATIC;
	fill_solid(&(_leds[0]), NUM_LEDS, color);
	FastLED.show();
}

void StatusVisualiser::blink(uint32_t phase, uint8_t iterations, uint32_t color, visualiserState_t transitionState)
{
	_animationStart = get_millisecond_timer();
	_animationPhase = (uint32_t) ((float) phase / 2.0);
	_animationIterations = iterations * 2;
	_blinkColor = color;
	if (_blinkColor == 0) _blinkColor = _defaultColor;
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

void StatusVisualiser::fillMeter(uint32_t fromT, uint32_t duration) {
	fillMeter(fromT, duration, _defaultColor);
}

void StatusVisualiser::fillMeter(uint32_t fromT, uint32_t duration, uint32_t colorCode) {
	_currentState = STATE_METER;
	_animationStart = fromT;
	_animationPhase = duration;
}

void StatusVisualiser::cylon(uint32_t beatLength) {
	cylon(_defaultColor, beatLength);
}

void StatusVisualiser::cylon(uint32_t color, uint32_t beatLength)
{
	_animationColor = color;
	_currentState = STATE_ANIMATION;
	_currentPattern = PATTERN_SPREAD;
}

void StatusVisualiser::nextPattern()
{
	_currentState = STATE_ANIMATION;
	int i = (int)_currentPattern + 1;
	if (i > (int)_maxPattern)
	{
		_currentPattern = PATTERN_OFF;
	}
	else
	{
		_currentPattern = (visualiserPattern_t)i;
	}
	startPattern(_currentPattern);
}

void StatusVisualiser::startPattern()
{
	_currentState = STATE_ANIMATION;
	_animationColor = _defaultColor;
	if (_currentPattern == PATTERN_OFF)
	{
		FastLED.clear(true);
	}
}

void StatusVisualiser::startPattern(visualiserPattern_t pattern) {
	_currentPattern = pattern;
	startPattern();
}

void StatusVisualiser::setProximityStatus(proximityStatus_t proxStat)
{
	if (proxStat == _proximity) return;

	_currentState = STATE_ANIMATION;
	switch (proxStat)
	{
	case PROXIMITY_GROUP:
		//change to group animation(s?)
		_maxPattern = PATTERN_MOVINGRAINBOW;
		break;
	case PROXIMITY_NEARBY:
		//change to nearby animation
		_maxPattern = PATTERN_STROBE;
		break;
	default:
		//change to last? alone animation
		_maxPattern = PATTERN_SPREAD;
		break;
	}

	startPattern(_maxPattern);
	_proximity = proxStat;
}

void StatusVisualiser::updateBeat(bool pressed)
{
	tapTempo.update(pressed);
	_beatLenghtMS = tapTempo.getBeatLength();
}

unsigned long StatusVisualiser::getBeatLength()
{
	_beatLenghtMS = tapTempo.getBeatLength();
	return _beatLenghtMS;
}

void StatusVisualiser::setBeatLength(unsigned long beatLengthMS)
{
	_beatLenghtMS = beatLengthMS;
	tapTempo.setBeatLength(beatLengthMS);
}