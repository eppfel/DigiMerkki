#pragma once

#include "Arduino.h"
#include <ArduinoJson.h>
#include <SPI.h>
#include <FS.h>
#include "SPIFFS.h"
#include <list>

#define CONFIG_KEY_PICS "pics"

#define CONFIG_MEMORY JSON_ARRAY_SIZE(NUM_BADGES*NUM_PICS) + JSON_OBJECT_SIZE(1) + 16
#define LOG_MEMORY 512 //JSON_ARRAY_SIZE(NUM_BADGES *NUM_PICS) + JSON_OBJECT_SIZE(1) + 16

template <typename T>
using SimpleList = std::list<T>;

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
    enum EventType
    {
        CONNECTION_EVT,
        SHARE_EVT,
        BEAT_EVT,
        POWER_EVT
    } type;
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
    void logBeatEvent(const uint32_t time,const int32_t &beat, const uint32_t &node);
    void logSharingEvent(const uint32_t time, const uint32_t &node, const int8_t &pic);
    void logConnectionEvent(const uint32_t time, const SimpleList<uint32_t> &nodes);
    void logEvent(const StaticJsonDocument<LOG_MEMORY> &doc);
};

