# ESP Mesh Bonding Interaction

This project automatically connects several ESP32 ~or ESP8266~ in range into a WiFi mesh. Through touch input devices can display and exchange pictures, show synchronised visualizations and track peoples face-to-face interactions.

## Dependencies

* [Painless Mesh](https://gitlab.com/painlessMesh/painlessMesh) (for connecting the ESPs into a mesh)
* [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) (for controlling the display on the TTGO T-display board)
* [eppfel/EasyButton (Custom Fork)](https://github.com/eppfel/EasyButton) (Button debounce and abstraction)
* [eppfel/ArduinoTapTempo (Custom Fork)](https://github.com/eppfel/ArduinoTapTempo) (To set beat from taps)
* [ESP Mesh Badge Protocol](https://version.aalto.fi/gitlab/digi-haalarit/esp-mesh-badge-protocol) (shared protocol specifications)
* Deprecated: [CapacitiveSensor](https://github.com/PaulStoffregen/CapacitiveSensor) (for ESP8266 touch sensing)

## Hardware

### Boards
The current implementation was implemented and tested on the [TTGO T-display (ESP32 dev module)](https://github.com/Xinyuan-LilyGO/TTGO-T-Display).

Other ESP32 boards are not supported anymore but could be by removing the calls to the TFT display.

ESP32 does only connect to 2.4GHz bands

### Periphery / Schematic

The program expects two capacitive touch buttons to be attached. Further, RGB LEDs are connected through 3.3V.
See pin definitions in the header file for the layout.

## Setup

All boards can immediately join the mesh without further setup or any electronics, but I/O is limited to the hardware buttons and display.

### T-display
This board does not feature an on-board LED, so an external has to be attached, to show connectivity. Further this board controls its integrated display.

### Build Process

1. You need `platformio` installed. I suggest https://platformio.org/platformio-ide (Arduino IDE does not support the build flags)
1. Clone this repository to your machine. `git clone git@version.aalto.fi:digi-haalarit/esp-mesh-bonding-interaction.git`
1. Open this repository in your platformio IDE (e.g. Visual Studio Code) or in your terminal `cd esp-mesh-bonding-interaction`
1. Run the `build` task in your IDE or `platformio run` in terminal (platformio automatically installs the dependencies on the first run)

### Upload procedure

Depending on the goal there are different ways to upload the software onto the ESP32 board.

#### Upload precompiled app with esptool

If you have a compiled binary, you only need the [esptool](https://github.com/espressif/esptool) to upload the board.

1. Install `python` and [esptool](https://github.com/espressif/esptool), if it is not already in your machine.
1. Check the port you want to upload to (e.g. `/dev/cu.usbserial-01E063F5`, you can get device with `pio device list`)
1. Run `esptool.py --port /dev/cu.usbserial-01E063F5 write_flash 0x10000 DigiMerkkiApp.bin` (With replacing the port and the correct path to `esptool.py`)

#### Upload from source inside Visual Studio Code

1. Set upload port in platformio.ini (you can get device with `pio device list`)
1. OPTIONAL: Run `Erase flash` (This deletes all data on the board and is needed for a clean start)
1. Run `Upload`
1. OPTIONAL: Run `Upload filesystem image` (This will delete files changed by the app, i.e. `configuration.json` and `interactionlog.json`. Hereafter the app also initialises the configuration.)

Working with multiple boards you can make use the scripts `uploadall.sh` or `eraseandupload.sh` to perform the upper steps for multiple boards. Just edit value of `ports` to filter `/dev/cu.usbserial-*` all boards connected to vis USB.

### Other boards not supported anymore

#### Huzzah32
At this point the Huzzah supports to control NeoPixel through FastLed library. 

#### Huzzah 8266
The feather with ESP8266 needs a `<500 OHM` resistor between two pins to sense capacititce touch but is not supported at the moment.

