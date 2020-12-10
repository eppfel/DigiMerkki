#include "FileStorage.h"

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

boolean FileStorage::loadConfiguration(BadgeConfig &config)
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
void FileStorage::saveConfiguration(const BadgeConfig &config)
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


void FileStorage::logEvent(BadgeEvent evt)
{ // list SPIFFS contents
    fs::File logFile = SPIFFS.open(LOG_FILE, FILE_WRITE);
    if (!logFile)
    {
        Serial.println("- failed to open configdirectoryu");
        return;
    }
    if (logFile.print("TEST"))
    {
        Serial.println("File was written");
    }
    else
    {
        Serial.println("File write failed");
    }
}