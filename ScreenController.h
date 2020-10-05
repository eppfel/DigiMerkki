#pragma once

#include "Arduino.h"
#include <FS.h>
#include "SPIFFS.h" // For ESP32 only
#include <SPI.h>
#include <TJpg_Decoder.h>
#include <TFT_eSPI.h>

void updateNumNodes(uint8_t numnodes);
void updateVoltage(float voltage);
void initScreen();
void displayMessage(String msg);
void showHomescreen();
void nextWallpaper();

extern TFT_eSPI tft; // Invoke custom TFT library