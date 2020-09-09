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
#define DISPLAY_ORIENTATION 3 //Rotation of the display in 90° steps form 0 to 3

#define TOUCHPIN T7       // Pin for sensing touch input 27
#define TOUCHPIN1 T9      // Pin for sensing touch input 32 (labelled as 33)
#define TOUCHPIN2 T8      // Pin for sensing touch input 33 (labelled as 32)
#define TTHRESHOLD 80     // threshold for touch
#define STHRESHOLD 20     // threshold for wake up touch
#define HW_BUTTON_PIN1 35 // Hardware button 1 on the T-display board
#define HW_BUTTON_PIN2 0  // Hardware button 1 on the T-display board

#define HANDSHAKETIME 10000 // time to perform a usccessful handshake between people

#define MESH_SSID "nopainnogain"
#define MESH_PASSWORD "istanbul"

#define ADC_PIN 34
