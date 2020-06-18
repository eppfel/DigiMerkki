//************************************************************
// This demo uses the painless Mesh library for connectivity and FastLED library to control the NeoPixel LEDs
//
// 1. blinks led once for every node on the mesh
// 2. blink cycle repeats every BLINK_PERIOD
// 3. sends a message to every node on the mesh when a capacitive touch event happened
// 4. prints anything it receives to Serial.print
// 5. Flashes the RGB LEDs when touch event is received
//
// TODO: Calibrate touch sensors on startup to avoid false positive touch events
//
//************************************************************
#include <painlessMesh.h>
#include <Filter.h>
#include <FastLED.h>

#if defined(ESP8266) // Feather Huzzah ESP8266
  #include <CapacitiveSensor.h>

  #define LED             0    // GPIO number of connected LED
  #define TOUCHPIN       13    // Pin for sensing touch input
  #define TOUCHPIN1      13    // Pin for sensing touch input
  #define TOUCHPIN2      13    // Pin for sensing touch input
  #define TTHRESHOLD   5000    // threshold for touch
  #define DATA_PIN       12    // Pin for controlling NeoPixel

#elif defined(ARDUINO_FEATHER_ESP32)
  #define LED            13    // GPIO number of connected LED
  #define TOUCHPIN       T0    // Pin for sensing touch input A5
  #define TOUCHPIN1      T9    // Pin for sensing touch input 32
  #define TOUCHPIN2      T8    // Pin for sensing touch input 33
  #define TTHRESHOLD     30    // threshold for touch
  #define DATA_PIN       12    // Pin for controlling NeoPixel

#else //ESP32 DEV Module
  #define LED            21    // GPIO number of connected LED
  #define TOUCHPIN       T3    // Pin for sensing touch input 15
  #define TOUCHPIN1      T9    // Pin for sensing touch input 32
  #define TOUCHPIN2      T8    // Pin for sensing touch input 33
  #define TTHRESHOLD     40    // threshold for touch
  #define DATA_PIN       12    // Pin for controlling NeoPixel

  #include <TFT_eSPI.h>
  #include <SPI.h>
  #ifndef TFT_DISPOFF
  #define TFT_DISPOFF  0x28
  #endif
  #ifndef TFT_SLPIN
  #define TFT_SLPIN    0x10
  #endif
  #define TFT_MOSI       19
  #define TFT_SCLK       18
  #define TFT_CS          5
  #define TFT_DC         16
  #define TFT_RST        23
  #define TFT_BL          4  // Display backlight control pin

#endif

#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for
#define   TOUCHDELAY      500  // delay to not spam with touch events

#define   MESH_SSID       "nopainnogain"
#define   MESH_PASSWORD   "istanbul"
#define   MESH_PORT       5555

#define   NUM_LEDS        5    // Number of LEDs conrolled through FastLED
#define   BS_PERIOD       120
#define   BS_COUNT        24

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;

#if defined(ESP8266) // Feather Huzzah
  CapacitiveSensor cap = CapacitiveSensor(4,TOUCHPIN); // 470k resistor between pins 4 & 2, pin 2 is sensor pin, add a wire and or foil if desired
#elif !defined(ARDUINO_FEATHER_ESP32) 
  TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom TFT library
#endif

CRGB leds[NUM_LEDS];// include variables for addresable LEDs
Task blinkBonding;
bool bsFlag = false;

ExponentialFilter<long> ADCFilter(5, TTHRESHOLD);
ExponentialFilter<long> ADCFilter1(5, TTHRESHOLD);
ExponentialFilter<long> ADCFilter2(5, TTHRESHOLD);
unsigned long lastTouch = 0;

#define BTN_A   1
#define BTN_B   2
#define BTN_C   4
#define BTN_AB  3
#define BTN_AC  5
#define BTN_BC  6
#define BTN_ABC 7

void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT);

  mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []() {
    // If on, switch off, else switch on
    if (onFlag)
      onFlag = false;
    else
      onFlag = true;
    blinkNoNodes.delay(BLINK_DURATION);

    if (blinkNoNodes.isLastIteration()) {
      // Finished blinking. Reset task for next run
      // blink number of nodes (including this node) times
      blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
      // Calculate delay based on current mesh time and BLINK_PERIOD
      // This results in blinks between nodes being synced
      blinkNoNodes.enableDelayed(BLINK_PERIOD -
                                 (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
    }
  });
  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

#if defined(ESP8266) // Feather Huzzah
  cap.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
#elif !defined(ARDUINO_FEATHER_ESP32)
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Started!", 0, 0);
#endif

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed  
  FastLED.clear();
  FastLED.setBrightness(64);
  blinkBonding.set(0, BS_COUNT, []() {

    for (int i = 0; i < NUM_LEDS; ++i)
    {
      if (i % 2 == bsFlag) {
        leds[i].setHue(224);
      } else {
        leds[i].setHue(128);
      }
    }
    bsFlag = !bsFlag;

    blinkBonding.delay(BS_PERIOD);
    if (blinkBonding.isLastIteration()) {
      FastLED.clear();
    }
  });
  userScheduler.addTask(blinkBonding);
  blinkBonding.enableDelayed(5000);

  randomSeed(analogRead(A0));
}

void loop() {
  mesh.update();

#if defined(ESP8266) // Feather Huzzah
  digitalWrite(LED, !onFlag);
#else // ESP32
  digitalWrite(LED, onFlag);
#endif

  if (checkForTouch() > 0 && lastTouch + TOUCHDELAY < millis()) { //check if touch event was broadcasted recently
      lastTouch = millis();
      sendMessage();
  }

  FastLED.show();
}

/* Check for touch input */
int checkForTouch() {
  int buttonState = 0;
#if defined(ESP8266) // Feather Huzzah
  long capval = cap.capacitiveSensor(30);
  ADCFilter.Filter(capval);
  if (ADCFilter.Current() > TTHRESHOLD) {  
    buttonState += 1;
  }
    
#else
  long capval = touchRead(TOUCHPIN);
  ADCFilter.Filter(capval);
  if (ADCFilter.Current() < TTHRESHOLD) {
    buttonState += BTN_A;
  }

  capval = touchRead(TOUCHPIN1);
  ADCFilter1.Filter(capval);
  if (ADCFilter1.Current() < TTHRESHOLD) {
    buttonState += BTN_B;
  }

  capval = touchRead(TOUCHPIN2);
  ADCFilter2.Filter(capval);
  if (ADCFilter2.Current() < TTHRESHOLD) {
    buttonState += BTN_C;
  }
    
#endif

  if (DEBUG && buttonState > 0) {
    switch (buttonState) {
      case BTN_A:
        Serial.print("Button press: A : ");
        Serial.println(ADCFilter.Current());
        break;
      case BTN_B:
        Serial.print("Button press: B : ");
        Serial.println(ADCFilter1.Current());
        break;
      case BTN_C:
        Serial.print("Button press: C : ");
        Serial.println(ADCFilter2.Current());
        break;
      case BTN_AB:
        Serial.print("Button press: A+B : ");
        Serial.println(ADCFilter.Current());
        Serial.print(", ");
        Serial.println(ADCFilter1.Current());
        break;
      case BTN_AC:
        Serial.print("Button press: A+C : ");
        Serial.print(ADCFilter.Current());
        Serial.print(", ");
        Serial.println(ADCFilter2.Current());
        break;
      case BTN_BC:
        Serial.print("Button press: B+C : ");
        Serial.print(ADCFilter1.Current());
        Serial.print(", ");
        Serial.println(ADCFilter2.Current());
        break;
      case BTN_ABC:
        Serial.print("Button press: A+B+C : ");
        Serial.print(ADCFilter.Current());
        Serial.print(", ");
        Serial.print(ADCFilter1.Current());
        Serial.print(", ");
        Serial.println(ADCFilter2.Current());
        break;
      default:
        Serial.println("No button was pressed, this message was not meant to be produced.");
        break;
    }
  }

  return buttonState;
}

/* Broadcast a touch event to all nodes*/
void sendMessage() {

  String msg = "Touch from node ";
  msg += mesh.getNodeId();
  msg += " myNodeTime: " + String(mesh.getNodeTime());

  Serial.printf("Sending message: %s\n", msg.c_str());
  Serial.println("");

  mesh.sendBroadcast(msg);

  if (calc_delay) {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end()) {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }

}

/* Controller for incoming messages that prints all incoming messages and listens on touch and bonding events */
void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  Serial.println("");

  if (msg.startsWith("Touch") && (lastTouch + TOUCHDELAY * 2) > millis()) { // check if another touch accordingly   
    Serial.printf("Bonding with %u\n", from);Serial.println("");
    mesh.sendSingle(from, "Bonding"); // send bonding handshake
  }else if (msg.startsWith("Bonding")) {
    Serial.printf("Bonded with %u\n", from);Serial.println("");
#if !defined(ESP8266) // ESP32
    blinkBonding.setIterations(BS_COUNT);
    blinkBonding.enable();
#endif
  }
}

void newConnectionCallback(uint32_t nodeId) {
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  Serial.println("");
  Serial.printf("--> startHere: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
  Serial.println("");
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println("");
  Serial.print("Nodetime: ");
  Serial.println(mesh.getNodeTime());
  calc_delay = true;
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}
