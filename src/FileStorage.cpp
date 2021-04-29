#include "FileStorage.h"
#include <StreamUtils.h>
#include <time.h>
#include <sys/time.h>

unsigned long getTime()
{
    time_t now;
    time(&now);
    return now;
}

// Prints the content of a file to the Serial
void FileStorage::printFile(const char *filename)
{
    // Open file for reading
    fs::File file = SPIFFS.open(filename);
    if (!file)
    {
        Serial.println(F("Failed to read file"));
        return;
    }

    // Extract each characters by one by one
    while (file.available())
    {
        Serial.print((char)file.read());
    }
    Serial.println();

    // Close the file
    file.close();
}

bool FileStorage::initConfiguration(badgeConfig_t &config, uint32_t nodeid)
{
    // Open file for reading
    fs::File file = SPIFFS.open(BADGES_FILE);

    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<BADGES_MEMORY> doc;

    DeserializationError error = deserializeJson(doc, file);
    // Deserialize the JSON document
    if (error)
    {
        Serial.println(F("Failed to read file, using default configuration"));
        file.close();
        return false;
    }

    // Iterate over all possible configruations
    uint8_t groupid;
    for (JsonObject elem : doc.as<JsonArray>())
    {
        //Check if this is this is the configuration for this device
        if (elem[CONFIG_KEY_ID] == nodeid) 
        {
            // Copy values from the JsonDocument to the Config
            groupid = elem[CONFIG_KEY_GROUP];
            config.color = (uint32_t) strtol(elem[CONFIG_KEY_COLOR], 0, 16);
            config.numPics = NUM_PICS;
            JsonArray pics = elem[CONFIG_KEY_PICS];
            for (size_t i = 0; i < NUM_PICS; i++)
            {
                config.pics[i] = pics[i];
            }
            break; // stop iterating, because we found the config
        }
    }

    size_t n = 0;
    for (JsonObject elem : doc.as<JsonArray>())
    {
        //Check if this is this is a node in the group but not itself
        if (elem[CONFIG_KEY_GROUP] == groupid && elem[CONFIG_KEY_ID] != nodeid)
        {
            // Add the id values from the JsonDocument to the config
            config.group[n++] = elem[CONFIG_KEY_ID];
        }
    }

    file.close();

    saveConfiguration(config);

    return true;
}

bool FileStorage::loadConfiguration(badgeConfig_t &config)
{
    // Open file for reading
    fs::File file = SPIFFS.open(CONFIG_FILE);

    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<CONFIG_MEMORY> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println(F("Failed to read file, using default configuration"));
        file.close();
        return false;
    } 
    // Copy values from the JsonDocument to the Config
    config.color = doc[CONFIG_KEY_COLOR];
    JsonArray groupnodes = doc[CONFIG_KEY_GROUP];
    JsonArray pics = doc[CONFIG_KEY_PICS];
    config.numPics = pics.size();
    size_t i = 0;
    for (JsonVariant pic : pics)
    {
        config.pics[i] = pic.as<uint8_t>();
        i++;
    }
    i = 0;
    for (JsonVariant node : groupnodes)
    {
        config.group[i] = node.as<uint32_t>();
        i++;
    }


    file.close();
    return true;
}

// Saves the configuration to a file
void FileStorage::saveConfiguration(const badgeConfig_t &config)
{
    // Delete existing file, otherwise the configuration is appended to the file
    SPIFFS.remove(CONFIG_FILE);

    // Open file for writing
    fs::File file = SPIFFS.open(CONFIG_FILE, FILE_WRITE);
    if (!file)
    {
        Serial.println(F("Failed to create file"));
        return;
    }

    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonDocument<CONFIG_MEMORY> doc;

    // Set the values in the document
    doc[CONFIG_KEY_COLOR] = config.color;
    JsonArray pics = doc.createNestedArray(CONFIG_KEY_PICS);
    for (size_t i = 0; i < config.numPics; i++)
    {
        pics.add(config.pics[i]);
    }
    JsonArray groupnodes = doc.createNestedArray(CONFIG_KEY_GROUP);
    for (size_t i = 0; i < MAX_GROUP_SIZE && config.group[i] != 0; i++)
    {
        groupnodes.add(config.group[i]);
    }

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0)
    {
        Serial.println(F("Failed to write to file"));
    }

    // Close the file
    file.close();
}

void FileStorage::logBootEvent(const uint32_t time)
{
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonDocument<LOG_MEMORY> doc;

    doc["t"] = time;
    doc["s"] = getTime();
    doc["e"] = BadgeEvent::POWER_EVT;

    logEvent(doc);
}

void FileStorage::logBeatEvent(const uint32_t time, const int32_t &beat, const uint32_t &node) {
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonDocument<LOG_MEMORY> doc;

    doc["t"] = time;
    doc["s"] = getTime();
    doc["e"] = BadgeEvent::BEAT_EVT;
    if (node != 0) doc["n"] = node;
    doc["b"] = beat;

    logEvent(doc);
}

void FileStorage::logPictureEvent(const uint32_t time, const int8_t &pic) {
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonDocument<LOG_MEMORY> doc;

    doc["t"] = time;
    doc["s"] = getTime();
    doc["e"] = BadgeEvent::PICTURE_EVT;
    doc["p"] = pic;

    logEvent(doc);
}

void FileStorage::logSharingEvent(const uint32_t time, const uint32_t &node, const int8_t &pic)
{
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonDocument<LOG_MEMORY> doc;

    doc["t"] = time;
    doc["s"] = getTime();
    doc["e"] = BadgeEvent::SHARE_EVT;
    doc["n"] = node;
    doc["p"] = pic;

    logEvent(doc);
}

void FileStorage::logConnectionEvent(const uint32_t time, const SimpleList<uint32_t> &nodes)
{
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonDocument<LOG_MEMORY> doc;

    doc["t"] = time;
    doc["s"] = getTime();
    doc["e"] = BadgeEvent::CONNECTION_EVT;

    nodes.begin();

    // Set the values in the document
    JsonArray nodesArr = doc.createNestedArray("n");
    SimpleList<uint32_t>::const_iterator node = nodes.begin();
    while (node != nodes.end())
    {
        nodesArr.add(*node);
        node++;
    }

    logEvent(doc);
}

void FileStorage::logEvent(const StaticJsonDocument<LOG_MEMORY> &doc)
{

    // Time recorded for test purposes
    // uint32_t t = millis();
    
    if (SPIFFS.usedBytes() > LOGGING_LIMIT) {
        Serial.println(F("Warning: Filesystem full. Logging halted."));
        return;
    }

    fs::File logFile = SPIFFS.open(LOG_FILE, FILE_APPEND);
    if (!logFile)
    {
        Serial.println("- failed to open log file");
        return;
    }

    WriteLoggingStream loggedFile(logFile, Serial);
    // Serialize JSON to file
    if (serializeJson(doc, loggedFile) == 0)
    {
        Serial.println("File write failed");
    }
    logFile.println();

    // Close the file
    logFile.close();

    // How much time did writing to the log take
    // t = millis() - t;
    // Serial.print(t);
    // Serial.println(" ms");
}