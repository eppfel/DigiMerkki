//************************************************************
// this is a simple example that uses the easyMesh library
//
// 1. blinks led once for every node on the mesh
// 2. blink cycle repeats every BLINK_PERIOD
// 3. sends a message to every node on the mesh when a capacitive touch event happened
// 4. prints anything it receives to Serial.print
//
//
//************************************************************
#include <painlessMesh.h>

#if defined(ESP8266) // Feather Huzzah ESP8266
  #include <CapacitiveSensor.h>
  #define LED             0    // GPIO number of connected LED
  #define ESP8266         1    // boolean for later checks
  #define TTHRESHOLD   3000    // threshold for touch

#elif defined(ARDUINO_FEATHER_ESP32)
  #include "stub.h"            // The capacitivesensor library does not compile in ESP32, so this stub is needed to not throw errors
  #define LED            13    // GPIO number of connected LED
  #define ESP8266         0    // boolean for later checks
  #define TOUCHPIN       T5    // Pin for sensing touch input
  #define TTHRESHOLD     40    // threshold for touch

#else //ESP32 DEV Module
  #include "stub.h"            // The capacitivesensor library does not compile in ESP32, so this stub is needed to not throw errors
  #define LED             2    // GPIO number of connected LED
  #define ESP8266         0    // boolean for later checks
  #define TOUCHPIN       T5    // Pin for sensing touch input
  #define TTHRESHOLD     40    // threshold for touch
  
#endif

#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for
#define   TOUCHDELAY      500  // delay to not spam with touch events

#define   MESH_SSID       "nopainnogain"
#define   MESH_PASSWORD   "istanbul"
#define   MESH_PORT       5555

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
#endif

int lastTouch = 0;

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

#if defined(ESP8266) // Feather Huzzah32
  cap.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
#else
  touchAttachInterrupt(TOUCHPIN, sendMessage, TTHRESHOLD);
#endif

  randomSeed(analogRead(A0));
}

void loop() {
  mesh.update();


#if defined(ESP8266) || defined(ARDUINO_FEATHER_ESP32)
  digitalWrite(LED, !onFlag);
#else
  digitalWrite(LED, onFlag);
#endif
  
#if defined(ESP8266) // Feather Huzzah32
  int capval = cap.capacitiveSensor(30);
  if (capval > TTHRESHOLD) sendMessage();
  //Serial.println(capval);                  // print capacitive sensor output
#endif

}

void sendMessage() {

  String msg = "Touch from node ";
  msg += mesh.getNodeId();
  msg += " myNodeTime: " + String(mesh.getNodeTime());

  Serial.printf("Sending message: %s\n", msg.c_str());
  Serial.println("");
  if (lastTouch + TOUCHDELAY > millis()) return;
  lastTouch = millis();

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


void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  Serial.println("");

  if (msg.startsWith("Touch") && (lastTouch + TOUCHDELAY * 2) > millis()) {    
    Serial.printf("Bonding with %u\n", from);Serial.println("");
    mesh.sendSingle(from, "Bonding");
  }else if (msg.startsWith("Bonding")) {
    Serial.printf("Bonded with %u\n", from);Serial.println("");
    //TODO: Push nodeId into a list to use for further interactions
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
