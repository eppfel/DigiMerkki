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

void FileStorage::initialise()
{
    // // Should load default config if run for the first time
    // Serial.println(F("Loading configuration..."));
    // loadConfiguration(config);

    // // Create configuration file
    // Serial.println(F("Saving configuration..."));
    // saveConfiguration(config);

    // // Dump config file
    // Serial.println(F("Print config file..."));
    // printFile(CONFIG_FILE);
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
    config.color = doc["color"];
    config.group = doc["group"];
    JsonArray pics = doc[CONFIG_KEY_PICS];
    config.numPics = pics.size();
    size_t i = 0;
    for (JsonVariant pic : pics)
    {
        config.pics[i] = pic.as<uint8_t>();
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
    doc["color"] = config.color;  
    doc["group"] = config.group;  
    JsonArray pics = doc.createNestedArray(CONFIG_KEY_PICS);
    for (size_t i = 0; i < config.numPics; i++)
    {
        pics.add(config.pics[i]);
    }

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0)
    {
        Serial.println(F("Failed to write to file"));
    }

    // Close the file
    file.close();
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

    // Close the file
    logFile.close();

    // How much time did writing to the log take
    // t = millis() - t;
    // Serial.print(t);
    // Serial.println(" ms");
}