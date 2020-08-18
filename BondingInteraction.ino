//************************************************************
// This demo uses the painless Mesh library for connectivity and FastLED library to control the NeoPixel LEDs
//
// 1. blinks led once for every node on the mesh
// 2. blink cycle repeats every BLINK_PERIOD
// 6. prints anything it receives to Serial.print
// 3. awaits user input for typing a 5 digit cypher on capacitive touch sensors
// 4. sends cypher to every node on the mesh
// 5. makes handshake with node that sends the same cypher
// 6. Flashes the RGB LEDs when when chaning mode and differently when bhandshake was successful
//
// TODO: Calibrate touch sensors on startup to avoid false positive touch events
//
//************************************************************
#include <painlessMesh.h>
#include "CapacitiveKeyboard.h"
#include "StatusVisualiser.h"

#ifdef ESP8266 // Feather Huzzah ESP8266
  #define LED             0    // GPIO number of connected LED
  #define TOUCHPIN       13    // Pin for sensing touch input
  #define TOUCHPIN1      13    // Pin for sensing touch input
  #define TOUCHPIN2      13    // Pin for sensing touch input
  #define SENDPIN         4    // Sending pulse from this pin to measure capacity
  #define TTHRESHOLD   5000    // threshold for touch

#elif defined(ARDUINO_FEATHER_ESP32)
  #define LED            13    // GPIO number of connected LED
  #define TOUCHPIN       33    // Pin for sensing touch input 33 (labelled 32)
  #define TOUCHPIN1      32    // Pin for sensing touch input 32 (labelled 33)
  #define TOUCHPIN2      15    // Pin for sensing touch input 15
  #define SENDPIN         4    // stub
#define TTHRESHOLD 30          // threshold for touch
#define STHRESHOLD 20          // threshold for wake up touch

#else //ESP32 DEV Module
  #define LED            21    // GPIO number of connected LED
  #define TOUCHPIN       T7    // Pin for sensing touch input 27
  #define TOUCHPIN1      T9    // Pin for sensing touch input 32 (labelled as 33)
  #define TOUCHPIN2      T8    // Pin for sensing touch input 33 (labelled as 32)
  #define SENDPIN         4    // stub
  #define TTHRESHOLD     30    // threshold for touch
  #define STHRESHOLD     20    // threshold for wake up touch 
  #define HW_BUTTON_PIN1 35    // Hardware button 1 on the T-display board
  #define HW_BUTTON_PIN2  0    // Hardware button 1 on the T-display board

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

EasyButton hwbutton1(HW_BUTTON_PIN1);
EasyButton hwbutton2(HW_BUTTON_PIN2);

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom TFT library
#endif

#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for

#define   HANDSHAKETIME   10000  // time to perform a usccessful handshake between people

#define   MESH_SSID       "nopainnogain"
#define   MESH_PASSWORD   "istanbul"

// RTC_DATA_ATTR int bootCount = 0; //store data in the RTC (persistent in deep sleep but not after reset)
// touch_pad_t touchPin; // for printing the touch pin

CapacitiveKeyboard touchInput(TOUCHPIN, TOUCHPIN1, TOUCHPIN2, TTHRESHOLD);

// Prototypes
void sleep();
void uploadUserData();
void buttonHandler(uint8_t keyCode);
void onPressed();
void sendCypher();
void sendMessage(String msg);
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

bool onFlag = false;
// @Override This function is called by FastLED inside lib8tion.h.Requests it to use mesg.getNodeTime instead of internal millis() timer.
uint32_t get_millisecond_timer_hook()
{
  return mesh.getNodeTime() / 1000;
}
StatusVisualiser visualiser(get_millisecond_timer_hook, 64);

// Task variables
#define TASK_CHECK_BUTTON_PRESS_INTERVAL 2     // in milliseconds
#define VISUALISATION_UPDATE_INTERVAL 1 // default scheduling time for currentPatternSELECT, in milliseconds
Task blinkNoNodes;
Task taskCheckButtonPress(TASK_CHECK_BUTTON_PRESS_INTERVAL, TASK_FOREVER, &checkButtonPress);
Task taskVisualiser(VISUALISATION_UPDATE_INTERVAL, TASK_FOREVER, &showVisualisations);

#define STATE_IDLE 0
#define STATE_CYPHER 1
uint8_t currentState = STATE_IDLE;

uint16_t cypher;
uint8_t  lastKey;
uint16_t cypherPeer;
uint32_t cypherNode;
uint32_t bondingStarttime = 0;
uint32_t bondingRequestNode;
uint32_t bondingSuccessNode;

void setup() {
  Serial.begin(115200);
  delay(1000);

  mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  touchInput.begin();
  touchInput.onPressed(buttonHandler, onPressed);

  userScheduler.addTask(taskCheckButtonPress);
  taskCheckButtonPress.enable();
  
  pinMode(LED, OUTPUT);
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

  userScheduler.addTask(taskVisualiser);
  taskVisualiser.enable();

#if !defined(ARDUINO_FEATHER_ESP32) && !defined(ESP8266)
  hwbutton1.begin();
  hwbutton2.begin();
  hwbutton1.onPressed(sleep);
  hwbutton2.onPressed(uploadUserData);

  tft.init();
  tft.setRotation(3);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Started!", 0, 0);
#endif

  randomSeed(analogRead(A0));
}

void onPressed() {
  touchInput.pressed();
}

void wakeup_callback()
{
  //placeholder callback function
}

// trigger deep sleep mode and wake up on any input from the touch buttons
void sleep()
{
  mesh.stop();
  Serial.println("Disconnected from mesh!");
  touchAttachInterrupt(TOUCHPIN, wakeup_callback, STHRESHOLD);
  touchAttachInterrupt(TOUCHPIN1, wakeup_callback, STHRESHOLD);
  touchAttachInterrupt(TOUCHPIN2, wakeup_callback, STHRESHOLD);
  esp_sleep_enable_touchpad_wakeup();
  Serial.println("Goind to sleep now!");
  esp_deep_sleep_start();
}

void uploadUserData() {
  static bool isFirstCall = true;
  Serial.println("Button 2 was pressed!");
  if (isFirstCall) {
    mesh.stop();
    Serial.println("Disconnected from mesh! Wait for 5 seconds to reconnect.");
    Task taskReconnectMesh(
        5000, TASK_ONCE, []() {
          Serial.println("Trying to reconnect to mesh!");
          mesh.initStation();
        },
        &userScheduler);
    isFirstCall = false;
  }
}

void typeCypher(uint8_t keyCode) {
  static uint8_t cypherLength = 0;
  if (keyCode == BTN_A || keyCode == BTN_B || keyCode == BTN_C) { // add buttunInput to cypher sequene
    visualiser.setMeter(cypherLength++);
    cypher = keyCode | (cypher << 3);
    if (DEBUG) {
      switch (keyCode) {
        case BTN_A:
          Serial.println("Add A to cypher");
          break;
        case BTN_B:
          Serial.println("Add B to cypher");
          break;
        case BTN_C:
          Serial.println("Add C to cypher");
          break;
        default:
          Serial.println("Adding nothing. This should not have been reached.");
        break;
      }
    }
#if !defined(ARDUINO_FEATHER_ESP32) && !defined(ESP8266)
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Cypher: " + cypherString(cypher), 0, 0);
#endif
  }
  if (keyCode == BTN_AC || cypherLength > 4) { //exit cyphertyping either if A+C Button were pressed OR if cypher is longer then 4 digits (12 bit) : cypher > 1 << 12
    
    if (cypher == 0) {// only send cypher, if it is not empty
      visualiser.blink(200, 1, CRGB::Blue);
#if !defined(ARDUINO_FEATHER_ESP32) && !defined(ESP8266)
      tft.fillScreen(TFT_BLACK);
      tft.drawString("No cypher sent.", 0, 0);
#endif
    } else {
      sendCypher();
      visualiser.blink(200, 3, CRGB::HotPink);
#if !defined(ARDUINO_FEATHER_ESP32) && !defined(ESP8266)
      tft.fillScreen(TFT_BLACK);
      tft.drawString("Sent cypher: " + cypherString(cypher), 0, 0);
#endif
    }

    Serial.println("Switch from cypher-input to idle");
    currentState = STATE_IDLE;
    cypherLength = 0;
  } 
}

void checkButtonPress() {
  touchInput.tick();
#if !defined(ARDUINO_FEATHER_ESP32) && !defined(ESP8266)
  hwbutton1.read();
  hwbutton2.read();
#endif
}

void showVisualisations() {
  visualiser.show();
#if defined(ESP8266) // Feather Huzzah
  digitalWrite(LED, !onFlag);
#else // ESP32
  digitalWrite(LED, onFlag);
#endif
}

void loop() {
  // EVERY_N_SECONDS(3) mesh.sendBroadcast("Touch debugging: " + String(touchRead(33)));
  mesh.update();
}

void buttonHandler(uint8_t keyCode)
{
  // static const boolean isCylon = false;

  switch (currentState)
  {
    case STATE_CYPHER:
      typeCypher(keyCode);
      break;
    default: //idle
      if (keyCode == BTN_AC)
      {
        currentState = STATE_CYPHER;
        cypher = 0;
        Serial.println("Switch from idle to cypher-input");
#if !defined(ARDUINO_FEATHER_ESP32) && !defined(ESP8266)
        tft.fillScreen(TFT_BLACK);
        tft.drawString("Cypher: ", 0, 0);
#endif
        visualiser.blink(200, 3, CRGB::HotPink);
      } else if (keyCode == BTN_B)
      {
        visualiser.cylon();
        // isCylon != isCylon;
        // if (isCylon) {
        // } else {
        //   visualiser.blink(0, 0, CRGB::Black);
        // }
      }
      break;
  }
}

String cypherString(uint16_t cypher) {
  String cypherword = "";
  while (cypher >= 1) {

    // Serial.println("in the loop");
    switch (cypher & 7) {
      case 1:
        cypherword = "A" + cypherword;
        break;
      case 2:
        cypherword = "B" + cypherword;
        break;
      case 4:
        cypherword = "C" + cypherword;
        break;
      default:
    // Serial.println("default case");
        break;
    }
    cypher = cypher >> 3;
    // Serial.println(cypherword);
  }
  return cypherword;
}

void sendCypher() {

  String msg = "Cypher : ";
  msg += String(cypher);

  sendMessage(msg);

  if ((bondingStarttime + HANDSHAKETIME) > millis()) {
    // bondingRequest was received earlier
    if (cypher == cypherPeer) {
      //cypher correct, so we should bond
      mesh.sendSingle(cypherNode, "Bonding"); // send bonding handshake
      bondingSuccessNode = cypherNode;
      Serial.printf("Bonding with %u\n", cypherNode);Serial.println("");
    } else {
      Serial.println("Our cypher does not match");
    }
  } else {
    //no bondingrequest active
    Serial.println("No open bonding request");
  }

  bondingStarttime = millis(); //bonding cypher by user so time is reset
}

/* Broadcast a message to all nodes*/
void sendMessage(String msg) {
  // String msg = "Touch from node ";
  // msg += mesh.getNodeId();
  // msg += " myNodeTime: " + String(mesh.getNodeTime());

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

//maybe move the handshake into a class whith one instance per Node to track states, instead of this if/else horror

/* Controller for incoming messages that prints all incoming messages and listens on touch and bonding events */
void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  Serial.println("");

  //FIX: remove the bonding cypher that was send after the response time winoow passed
  if (msg.startsWith("Cypher")) { // check if another touch accordingly   

    cypherPeer = msg.substring(9).toInt(); //other devices sending cyphers will override this value and block bonding -> connect to nodeid, because that is unique
    cypherNode = from;

    Serial.println("Bonding cypher " + cypherString(cypherPeer) + " received from " + from);

    if ((bondingStarttime + HANDSHAKETIME) > millis()) {
      
      //had started a request before
      if (cypher == cypherPeer) {
         // bonding cypher correct, so let's bond
        mesh.sendSingle(from, "Bonding");
        Serial.printf("Bonding with %u\n", from);Serial.println("");
        bondingSuccessNode = from;
        if (bondingRequestNode == bondingSuccessNode) {
          Serial.println("Bonded with" + from);
          visualiser.cylon();
          //store in list
        }
      } else {
        Serial.println("Their cypher does not match");
      }
    } else {
      //bonding was out of time or no request from this side
      Serial.println("No open request, yet");
    }
    bondingStarttime = millis();
  } else if (msg.startsWith("Bonding")) {
    if (bondingSuccessNode == from) { //this has to be checked against a time
      Serial.println("Bonded with" + from);
      visualiser.cylon();
    } else {
      bondingRequestNode = from;
    }
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
