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
// #include "headers/tdisplay.h"

#include <painlessMesh.h>
#include "CapacitiveKeyboard.h"
#include "StatusVisualiser.h"
#include "ScreenController.h"

// Prototypes
void checkBatteryCharge();
void goToSleep();
void uploadUserData();
void broadcastCapSense();
void buttonHandler(uint8_t keyCode);
void onPressed();
void startBonding();
void sendMessage(String msg);
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);
 
EasyButton hwbutton1(HW_BUTTON_PIN1);
EasyButton hwbutton2(HW_BUTTON_PIN2);
CapacitiveKeyboard touchInput(TOUCHPIN_LEFT, TOUCHPIN_RIGHT, TTHRESHOLD);

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

// @Override This function is called by FastLED inside lib8tion.h.Requests it to use mesg.getNodeTime instead of internal millis() timer.
uint32_t get_millisecond_timer_hook()
{
  return mesh.getNodeTime() / 1000;
}
StatusVisualiser visualiser(get_millisecond_timer_hook, 64);

// Task variables
Task taskCheckBattery(BATTERY_CHARGE_CHECK_INTERVAL, TASK_FOREVER, &checkBatteryCharge);
Task taskCheckButtonPress(TASK_CHECK_BUTTON_PRESS_INTERVAL, TASK_FOREVER, &checkButtonPress);
Task taskVisualiser(VISUALISATION_UPDATE_INTERVAL, TASK_FOREVER, &showVisualisations);
Task taskShowLogo(LOGO_DELAY, TASK_ONCE, &showHomescreen);

#define STATE_IDLE 0
#define STATE_BONDING 1
#define STATE_SCORE 2
#define STATE_PROXIMITY 3
#define STATE_GROUP 4
uint8_t currentState = STATE_IDLE;

#define BONDING_IDLE 0
#define BONDING_REQUESTED 1
#define BONDING_STARTED 2
#define BONDING_INPROGRESS 3
uint8_t bondingState = BONDING_IDLE;

struct bondingRequest_t {
  uint32_t node;
  uint32_t startt;
};

SimpleList<bondingRequest_t> bondingCandidates;
bondingRequest_t bondingCandidate;

void setup() {
  Serial.begin(115200);
  delay(25);

  // Start Display for Feedback first
  initScreen();

  checkBatteryCharge();

  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS initialisation failed!");
    while (1)
      yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nSPIFFS initialised.");

  mesh.setDebugMsgTypes(ERROR | DEBUG); // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  userScheduler.addTask(taskShowLogo);
  userScheduler.addTask(taskCheckBattery);
  taskCheckBattery.enableDelayed(BATTERY_CHARGE_CHECK_INTERVAL);

  touchInput.begin();
  touchInput.onPressed(buttonHandler, onPressed);

  userScheduler.addTask(taskCheckButtonPress);
  taskCheckButtonPress.enable();

  userScheduler.addTask(taskVisualiser);
  taskVisualiser.enable();

  hwbutton1.begin();
  hwbutton2.begin();
  hwbutton1.onPressed(pressedShutdown);
  hwbutton2.onPressed(showVoltage);

  displayMessage("Here we go!");
  taskShowLogo.restartDelayed(2000);

  randomSeed(analogRead(A0));
}

void loop()
{
  mesh.update();
}

/*
* POWER MANAGEMENT
*/

void wakeup_callback()
{
  //placeholder callback function
}

// Check if on Battery and empty, if so go to sleep to protect boot loop on low voltage
void checkBatteryCharge()
{
  if (getInputVoltage() < 3.0)
  {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Got no juice :(", tft.width() / 2, tft.height() / 2);
    goToSleep();
  }
}

int vref = 1100;
float getInputVoltage()
{
  uint16_t vPin = analogRead(ADC_PIN);
  float v = ((float)vPin / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
  updateVoltage(v);
  return v;
}

void showVoltage()
{
  String voltage = "Voltage :" + String(getInputVoltage()) + "V";
  Serial.println(voltage);
  displayMessage(voltage);
  taskShowLogo.restartDelayed();
}

void toggleDisplay()
{
  Serial.println("Turning display off");
  int r = digitalRead(TFT_BL);
  Serial.println(r);
  digitalWrite(TFT_BL, !r);

  // tft.writecommand(TFT_DISPOFF);
  // tft.writecommand(TFT_SLPIN);
}

void pressedShutdown()
{
  displayMessage("Shutting down");
  goToSleep();
}

// trigger deep sleep mode and wake up on any input from the touch buttons
void goToSleep()
{
  mesh.stop();
  Serial.println("Disconnected from mesh!");
  visualiser.turnOff();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)HW_BUTTON_PIN1, 0);
  Serial.println("Goind to sleep in 2 sec!");
  delay(2000);
  esp_deep_sleep_start();
}

/*
* USER INPUT
*/

void onPressed()
{
  touchInput.pressed();
}

void checkButtonPress()
{
  touchInput.tick();
  hwbutton1.read();
  hwbutton2.read();
}

void uploadUserData()
{
  static bool isFirstCall = true;
  Serial.println("Button 2 was pressed!");
  if (isFirstCall)
  {
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

void broadcastCapSense()
{
  String msg = String(touchRead(TOUCHPIN_LEFT)) + " : " + String(touchRead(TOUCHPIN_RIGHT));
  mesh.sendBroadcast(msg);
}

void buttonHandler(uint8_t keyCode)
{
  switch (currentState)
  {
  case STATE_BONDING:
    // fill meter based on time
    if (keyCode == TAP_BOTH)
    {
      abortBonding();
      currentState = STATE_IDLE;
    }
    break;
  default: //idle

    Serial.println("Tap " + String(keyCode));
    if (keyCode == HOLD_BOTH)
    {
      // currentState = STATE_CYPHER;
      Serial.println("Switch from idle to cypher-input");
      taskShowLogo.disable();
      displayMessage("Wating for partner...");
      visualiser.blink(300, 99, CRGB::White, StatusVisualiser::STATE_ANIMATION);
    }
    else if (keyCode == TAP_LEFT)
    {
      visualiser.nextPattern();
    }
    else if (keyCode == TAP_RIGHT)
    {
      nextWallpaper();
    } else if (keyCode == TAP_BOTH) {
      currentState = STATE_BONDING;
      startBonding();
    }
    break;
  }
}

/*
* HANDLE BONDING
*/

void startBonding()
{
  displayMessage("Search Peer");
  taskShowLogo.restartDelayed();

  sendMessage("BRQA");
  bondingState = BONDING_REQUESTED;

  if(bondingCandidates.empty()) {
    Serial.println("No Requests stored");
  }
  else // bondingRequest was received earlier
  {
    while(!bondingCandidates.empty() && bondingState != BONDING_STARTED)
    {
      bondingCandidate = bondingCandidates.front();
      if ((bondingCandidate.startt + BONDINGTIMEOUT) > mesh.getNodeTime()) {
        bondingCandidates.pop_front();
        Serial.println("Request timeout");
      } else {
        mesh.sendSingle(bondingCandidate.node, "BRQS"); // send bonding handshake
        bondingState = BONDING_STARTED;
        // start timer based on
        Serial.printf("Bonding requested from %u\n", bondingCandidate.node);
      }
    }
  }
}

void abortBonding()
{
  displayMessage("Abort");
  taskShowLogo.restartDelayed();
  sendMessage("BRAB");
  bondingState = BONDING_IDLE;
  if (!bondingCandidates.empty()) bondingCandidates.pop_front();
  Serial.println("Bonding aborted by user");
}

void handleBonding(uint32_t from, String &msg)
{
  if (msg.startsWith("BRQA")) {

    // Add new request to the cue
    bondingRequest_t newCandidate;
    newCandidate.node = from;
    newCandidate.startt = mesh.getNodeTime();
    bondingCandidates.remove_if([&from](bondingRequest_t c) { return c.node == from; });

    if (bondingState == BONDING_REQUESTED) { //user had requested before, so start bonding
      bondingCandidate = newCandidate;
      bondingCandidates.push_front(newCandidate);
      mesh.sendSingle(bondingCandidate.node, "BRQS"); // send bonding handshake
      bondingState = BONDING_STARTED;
      Serial.printf("Bonding requested from %u\n", bondingCandidate.node);
    }
    else //save for later
    {
      bondingCandidates.push_back(newCandidate);
      Serial.printf("User has not initiated Bonding yet!");
    }
  }
  else if (msg.startsWith("BRAB")) { // Abort ercevied
    if (bondingState == BONDING_STARTED && bondingCandidate.node == from) { // abort current bonding procedure
      //visualise abort
      bondingState = BONDING_IDLE;
      bondingCandidates.pop_front();
    }
    else //remove frim request cue
    {
      bondingCandidates.remove_if([&from](bondingRequest_t c) { return c.node == from; });
    }
  }
  else if (msg.startsWith("BRQS"))
  {
    if (bondingState == BONDING_REQUESTED) // if bonding hasnt strted but was requested, skip start and bond immediatly
    {
      bondingCandidate.node = from;
      bondingCandidate.startt = mesh.getNodeTime();
      bondingCandidates.push_front(bondingCandidate);
      mesh.sendSingle(bondingCandidate.node, "BRQS"); // send bonding handshake
      bondingState = BONDING_INPROGRESS;
      Serial.printf("Bonding now with %u\n", bondingCandidate.node);
      visualiser.blink(500, 3, CRGB::Green, StatusVisualiser::STATE_ANIMATION);
      // start timer based on
    }
    else if (bondingState == BONDING_STARTED && bondingCandidate.node == from) // we started first and now 
    {
      //start animation and timer
      bondingState = BONDING_INPROGRESS;
      Serial.printf("Bonding now with %u\n", bondingCandidate.node);
      visualiser.blink(500, 3, CRGB::Green, StatusVisualiser::STATE_ANIMATION);
      // start timer based on
    }
    else {
      // user did mno start it, so discard this? Or shouled we store it? 
      Serial.println("User hasn't started bonding or is in progtress with other peer, so we do not react");
    }
  }
}

/*
* OUTPUT
*/

void showVisualisations()
{
  visualiser.show();
}

/*
* NETWORKING
*/

/* Broadcast a message to all nodes*/
void sendMessage(String msg)
{
  // String msg = "Touch from node ";
  // msg += mesh.getNodeId();
  // msg += " myNodeTime: " + String(mesh.getNodeTime());

  Serial.printf("Sending message: %s\n", msg.c_str());

  mesh.sendBroadcast(msg);

  if (calc_delay)
  {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end())
    {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }
}

/* Controller for incoming messages that prints all incoming messages and listens on touch and bonding events */
void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  Serial.println("");

  String protocol = msg.substring(0,4);
  if (protocol == "BRQA" || protocol == "BRAB" || protocol == "BRQS")
  {
    handleBonding(from, msg);
  }
  else if (msg.startsWith("Anythgin else"))
  {
    //do anything else based on network
  }
}

void newConnectionCallback(uint32_t nodeId)
{
  // Display change on tft

  Serial.printf("New Connection, nodeId = %u", nodeId);
  // Serial.printf("--> startHere: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
  // Serial.println("");
  updateNumNodes(nodes.size());
  showHomescreen();
}

void changedConnectionCallback()
{
  Serial.printf("Changed connections\n");
  // Display changes on tft

  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end())
  {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println("");
  Serial.print("Nodetime: ");
  Serial.println(mesh.getNodeTime());
  calc_delay = true;

  updateNumNodes(nodes.size());
  showHomescreen();
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay)
{
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}
