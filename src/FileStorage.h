#pragma once

#include "Arduino.h"
#include <ArduinoJson.h>
#include <SPI.h>
#include <FS.h>
#include "SPIFFS.h"
#include <list>

// JSON file format key specifications
#define CONFIG_KEY_ID "id"
#define CONFIG_KEY_GROUP "group"
#define CONFIG_KEY_COLOR "color"
#define CONFIG_KEY_PICS "pics"

#define BADGES_MEMORY NUM_BADGES * JSON_ARRAY_SIZE(NUM_PICS) + JSON_ARRAY_SIZE(NUM_BADGES) + NUM_BADGES * JSON_OBJECT_SIZE(4) + NUM_BADGES * 8 + 32
#define CONFIG_MEMORY JSON_ARRAY_SIZE(NUM_BADGES*NUM_PICS) + JSON_OBJECT_SIZE(3) + 16
#define LOG_MEMORY 512 //JSON_ARRAY_SIZE(NUM_BADGES *NUM_PICS) + JSON_OBJECT_SIZE(1) + 16

template <typename T>
using SimpleList = std::list<T>;

struct badges_t
{
    uint32_t node;
    uint8_t group;
    uint32_t color;
    uint8_t pics[NUM_PICS];
};

struct badgeConfig_t
{
    size_t numPics;
    uint32_t group[MAX_GROUP_SIZE];
    uint32_t color;
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
        POWER_EVT,
        PICTURE_EVT
    } type;
};

class FileStorage
{
public:
    FileStorage(){}
    ~FileStorage(){}
    
    void printFile(const char *filename);
    bool initConfiguration(badgeConfig_t &config, uint32_t nodeid);
    bool loadConfiguration(badgeConfig_t &config);
    void saveConfiguration(const badgeConfig_t &config);
    void logBootEvent(const uint32_t time);
    void logBeatEvent(const uint32_t time,const int32_t &beat, const uint32_t &node);
    void logPictureEvent(const uint32_t time, const int8_t &pic);
    void logSharingEvent(const uint32_t time, const uint32_t &node, const int8_t &pic);
    void logConnectionEvent(const uint32_t time, const SimpleList<uint32_t> &nodes);
    void logEvent(const StaticJsonDocument<LOG_MEMORY> &doc);
};

