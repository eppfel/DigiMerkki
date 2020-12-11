#define USER_SETUP_LOADED 1
#define ST7789_DRIVER 1
#define TFT_WIDTH 135
#define TFT_HEIGHT 240
#define CGRAM_OFFSET 1
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 23
#define TFT_BL 4
#define TFT_BACKLIGHT_ON HIGH
#define LOAD_GLCD 1
#define LOAD_FONT2 1
#define LOAD_FONT4 1
#define LOAD_FONT6 1
#define LOAD_FONT7 1
#define LOAD_FONT8 1
#define LOAD_GFXFF 1
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 6000000 // 6 MHz is the maximum SPI read speed for the ST7789V

#define TOUCHPIN_LEFT 13       // Pin for sensing touch input A
#define TOUCHPIN_RIGHT 2      // Pin for sensing touch input B
#define DEBOUNCE_TIME 50
#define TTHRESHOLD 60     // threshold for touch
#define HW_BUTTON_PIN1 35 // Hardware button 1 on the T-display board
#define HW_BUTTON_PIN2 0  // Hardware button 1 on the T-display board

#define ADC_PIN 34

#define NUM_LEDS 5 // Number of LEDs conrolled through FastLED
#define NEOPIXEL_PIN 12