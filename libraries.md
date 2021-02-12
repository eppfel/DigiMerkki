# Libraries to consider:

## Task management

https://gitter.im/painlessMesh/Lobby?at=5f1d733777eccd0e1476ffbf
https://techtutorialsx.com/2017/05/06/esp32-arduino-creating-a-task/
https://www.freertos.org/FreeRTOS-quick-start-guide.html

## Power managament
https://github.com/pangodream/18650CL

### Sleep / power saving modes
* https://platformio.org/lib/show/401/Adafruit%20SleepyDog%20Library / https://github.com/adafruit/Adafruit_SleepyDog
https://savjee.be/2019/12/esp32-tips-to-increase-battery-life/

### WiFi on / off

## User Data tracking / storage

### SQLite
https://platformio.org/lib/show/6173/Sqlite3Esp32 / https://github.com/siara-cc/esp32_arduino_sqlite3_lib most useful WINNER
https://github.com/siara-cc/sqlite_micro_logger_arduino/tree/master/examples/ESP32_Console for logging sensor data

### Internal Flash Memory / SPIFFS
- https://github.com/Marzogh/SPIMemory, low level flash memory
- https://github.com/greiman/SdFat-beta SD cards, ExFat support for large files
https://platformio.org/lib/show/6609/SdFat%20-%20Adafruit%20Fork
https://platformio.org/lib/show/1643/Adafruit%20SPIFlash
https://platformio.org/lib/show/322/SdFat

### LittleFS:
https://github.com/me-no-dev/ESPAsyncWebServer/issues/792
https://github.com/lorol/arduino-esp32fs-plugin
https://community.platformio.org/t/how-to-define-littlefs-partition-build-image-and-flash-it-on-esp32/11333/26

### Data upload
???

## User input

### TapTempo
https://github.com/dxinteractive/ArduinoTapTempo

### Touch input:

https://github.com/JSC-electronics/ObjectButton : Promising approach that implements Interfaces, so no callbacks needed and buttons give their reference (see two buttons example)
https://github.com/bxparks/AceButton : Extensive button library, with capacitive sensor button example
https://github.com/nathanRamaNoodles/SensorToButton : Simple presses with any sensor input
https://github.com/mathertel/OneButton : Did not see an easy way to integrate touch buttons (old)

https://github.com/admarschoonen/TouchLib : specific for wearable projects

### Force Resitive Sensor / Pressure Sensor / Velostat

https://github.com/billythemusical/textile-interfaces/blob/master/velo-test/velo-test.ino
https://www.makerguides.com/fsr-arduino-tutorial/


## Graphics

### LEDs
https://github.com/atuline/FastLED-Demos

# Snippets

```c
long filterExponentially(long newVal, long currentVal, int weight) {
  return (100 * weight * newVal + (100 - weight) * currentVal + 50)/10000;
}

visualiser.blink(360, 8, CRGB::DeepSkyBlue);
```

# Tools
https://github.com/gshubham586/Mesh_Network_Visualisation

https://cookierobotics.com/040/