# ESP Mesh Bonding Interaction

This project automatically connects several ESP32 or ESP8266 in range into a WiFi mesh. Through touch input pairs of devices can "bond" together to track peoples face-to-face interactions or show synchronised visulisations.

## Dependencies

* [Painless Mesh](https://gitlab.com/painlessMesh/painlessMesh)
* [CapacitiveSensor](https://github.com/PaulStoffregen/CapacitiveSensor) (for ESP8266 touch sensing)
* [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) (for controlling the display on the TTGO T-display board)
* [EasyButton (Custom Fork)]

## Hardware

### Boards
The current implementation was implemetned and tested on the [TTGO T-display (ESP32 dev module)](https://github.com/Xinyuan-LilyGO/TTGO-T-Display).

Other ESP32 boards are not supported anymore but could be by rmeoving the calls to the TFT display.

ESP32 does only connect to 2.4GHz bands

### Periphery / Schematic

The program expects three capacitive touch buttons to be attached. Further, RGB LEDs are connected through 5V.
See pin defintions in the header file for the layout.

## Setup

All boards can immediatly join the mesh without further setup or any elcectronics, but the will not use any I/O.

### T-display
This board does not feature an onboard LED, so an external has to be attached, to show connectivity. Further this board controls its integrated display.

### Orther boards not supported anymore

#### Huzzah32
At this point the Huzzah supports to control NeoPixel through FastLed library. 

#### Huzzah 8266
The feather with ESP8266 needs a `<500 OHM` resistor between two pins to sense capacititce touch but is not supported at the moment.

