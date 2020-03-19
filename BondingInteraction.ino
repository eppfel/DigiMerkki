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

#if defined(ESP8266) // Feather Huzzah ESP8266
  #include <CapacitiveSensor.h>
  #define LED             0    // GPIO number of connected LED
  #define TTHRESHOLD   5000    // threshold for touch

#elif defined(ARDUINO_FEATHER_ESP32)
  #include <FastLED.h>
  #define LED            13    // GPIO number of connected LED
  #define TOUCHPIN       T6    // Pin for sensing touch input
  #define TTHRESHOLD     42    // threshold for touch

#else //ESP32 DEV Module
  #include <FastLED.h>
  #define LED             2    // GPIO number of connected LED
  #define TOUCHPIN       T6    // Pin for sensing touch input
  #define TTHRESHOLD     40    // threshold for touch
  
#endif

#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for
#define   TOUCHDELAY      500  // delay to not spam with touch events

#define   MESH_SSID       "nopainnogain"
#define   MESH_PASSWORD   "istanbul"
#define   MESH_PORT       5555

#define   DATA_PIN       12    // Pin for controlling NeoPixel
#define   NUM_LEDS        2    // Number of LEDs conrolled through FastLED
#define   BS_PERIOD      120
#define   BS_COUNT       24

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
  CapacitiveSensor cap = CapacitiveSensor(4,13); // 470k resistor between pins 4 & 2, pin 2 is sensor pin, add a wire and or foil if desired
#else // ESP32// Task to blink the number of nodes
  Task blinkBonding;
  CRGB leds[NUM_LEDS];
  bool bsFlag = false;
#endif


ExponentialFilter<long> ADCFilter(5, TTHRESHOLD);
unsigned long lastTouch = 0;

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
#else //ESP32
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed  
  FastLED.clear();
  FastLED.setBrightness(64);
  blinkBonding.set(0, BS_COUNT, []() {

    // leds[bsFlag] = CRGB::Pink;
    // leds[!bsFlag] = CRGB::Cyan;
    leds[bsFlag].setHue(224);
    leds[!bsFlag].setHue(128);
    bsFlag = !bsFlag;

    blinkBonding.delay(BS_PERIOD);
    if (blinkBonding.isLastIteration()) {
      FastLED.clear();
    }
  });
  userScheduler.addTask(blinkBonding);
  blinkBonding.enableDelayed(5000);
#endif

  randomSeed(analogRead(A0));
}

void loop() {
  mesh.update();

#if defined(ESP8266) // Feather Huzzah
  digitalWrite(LED, !onFlag);
#else // ESP32
  digitalWrite(LED, onFlag);
#endif

  checkForTouch();

#if defined(ARDUINO_FEATHER_ESP32)
  FastLED.show();
#endif
}

/* Check for touch input */
void checkForTouch() {
#if defined(ESP8266) // Feather Huzzah
  long capval = cap.capacitiveSensor(30);
  ADCFilter.Filter(capval);
  if (ADCFilter.Current() > TTHRESHOLD) {  
    
#else
  long capval = touchRead(TOUCHPIN);
  ADCFilter.Filter(capval);
  if (ADCFilter.Current() < TTHRESHOLD) {
    
#endif

    Serial.print("Touch: ");                  // print capacitive sensor output
    Serial.println(ADCFilter.Current() );
    if (lastTouch + TOUCHDELAY < millis()) { //check if touch event was broadcasted recently
      lastTouch = millis();
      sendMessage();
    }
  }  
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
#ifdef ESP32 // ESP32
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
