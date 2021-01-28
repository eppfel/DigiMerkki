#pragma once

#include "Arduino.h"
#include <FS.h>
#include "SPIFFS.h" // For ESP32 only
#include <SPI.h>
#include <TJpg_Decoder.h>
#include <TFT_eSPI.h>
#include "FileStorage.h"

void updateNumNodes(uint8_t numnodes);
void updateVoltage(float voltage);
void initScreen();
void displayMessage(String msg);
void showHomescreen();
void nextPicture();
size_t getCurrentPicture();
void setCurrentPicture(size_t picId);

extern TFT_eSPI tft; // Invoke custom TFT library
extern BadgeConfig configuration;