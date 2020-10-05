#include "ScreenController.h"

const static uint8_t NUM_WALLPAPERS = 5;
char const *imgfiles[] = {"Baboon", "bmo", "nude", "aalto", "octopus", "digital-haalarit"};
RTC_DATA_ATTR int8_t wallpaper = 1;

TFT_eSPI tft(TFT_WIDTH, TFT_HEIGHT); // Invoke custom TFT library

uint8_t _numnodes = 0;
float _voltage = 0;

// This next function will be called during decoding of the jpeg file to
// render each block to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    // Stop further decoding as image is running off bottom of screen
    if (y >= tft.height())
        return 0;

    // This function will clip the image block rendering automatically at the TFT boundaries
    tft.pushImage(x, y, w, h, bitmap);

    // This might work instead if you adapt the sketch to use the Adafruit_GFX library
    // tft.drawRGBBitmap(x, y, bitmap, w, h);

    // Return 1 to decode next block
    return 1;
}

void initScreen()
{
    // Start Display for Feedback first
    tft.init();
    tft.setRotation(DISPLAY_ORIENTATION);
    tft.setSwapBytes(true);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);

    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);

    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Just a sec...", tft.width() / 2, tft.height() / 2);
}

void updateNumNodes(uint8_t numnodes) {
    _numnodes = numnodes;
}

void updateVoltage(float voltage) {
    _voltage = voltage;
}

void displayMessage(String msg)
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(msg, tft.width() / 2, tft.height() / 2);
}

void nextWallpaper() {
    wallpaper++;
    if (wallpaper >= NUM_WALLPAPERS)
    {
        wallpaper = 0;
    }
    showHomescreen();
}

void showHomescreen()
{
    // Time recorded for test purposes
    // uint32_t t = millis();

    char picturefilename[24];
    sprintf(picturefilename, "/%s.jpg", imgfiles[wallpaper]);
    TJpgDec.drawFsJpg(0, 0, picturefilename);
    yield();

    //Status bar
    if (_numnodes > 0)
    {
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.setTextDatum(BL_DATUM);
        tft.drawString(String(_numnodes) + " close", 0, tft.height());
        tft.setTextColor(TFT_WHITE);
    }
    else
    {
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.setTextDatum(BL_DATUM);
        tft.drawString("No one around", 0, tft.height());
        tft.setTextColor(TFT_WHITE);
    }

    if (_voltage < 3.3)
    {
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.setTextDatum(BR_DATUM);
        tft.drawString("Battery low", tft.width(), tft.height());
        tft.setTextColor(TFT_WHITE);
    }

    // How much time did rendering take (ESP8266 80MHz 271ms, 160MHz 157ms, ESP32 SPI 120ms, 8bit parallel 105ms
    // t = millis() - t;
    // Serial.print(t);
    // Serial.println(" ms");
}