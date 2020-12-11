#pragma once

#include "Arduino.h"
#include <ArduinoJson.h>
#include <SPI.h>
#include <FS.h>
#include "SPIFFS.h"

#define CONFIG_KEY_PICS "pics"
#define CONFIG_MEMORY JSON_ARRAY_SIZE(NUM_BADGES*NUM_PICS) + JSON_OBJECT_SIZE(1) + 16

struct badges_t
{
    uint32_t node;
    uint8_t group;
    uint8_t Pics[NUM_PICS];
};

struct BadgeConfig
{
    size_t numPics;
    uint8_t pics[NUM_BADGES * NUM_PICS];
};

struct BadgeEvent
{
    time_t time;
    char name[24];
    uint32_t connection;
};

class FileStorage
{
public:
    FileStorage(){}
    ~FileStorage(){}
    
    void initialise();
    void printFile(const char *filename);
    bool loadConfiguration(BadgeConfig &config);
    void saveConfiguration(const BadgeConfig &config);
    void logEvent(BadgeEvent evt);
private:
};

