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
void setTempo();
void checkDeviceStatus();
void buttonHandler(uint8_t keyCode);
void onPressed();
void userStartBonding();
void sendBondingPing();
void sendMessage(String msg);
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);
 
EasyButton hwbutton1(HW_BUTTON_PIN1);
EasyButton hwbutton2(HW_BUTTON_PIN2);
CapacitiveKeyboard touchInput(TOUCHPIN_LEFT, TOUCHPIN_RIGHT, TTHRESHOLD);
RTC_DATA_ATTR boolean freshStart = true;

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
Task taskBondingPing(BONDINGPING, TASK_FOREVER, &sendBondingPing);
Task taskSendBPM(TAPTIME,TASK_ONCE);
Task taskReconnectMesh(TAPTIME, TASK_ONCE);

#define STATE_IDLE 0
#define STATE_BONDING 1
#define STATE_BOND 2
#define STATE_SCORE 3
#define STATE_PROXIMITY 4
#define STATE_GROUP 5
#define STATE_TAPTEMPO 6
uint8_t currentState = STATE_IDLE;

#define BONDING_IDLE 0
#define BONDING_REQUESTED 1
#define BONDING_STARTED 2
#define BONDING_INPROGRESS 3
#define BONDING_COMPLETE 4
uint8_t bondingState = BONDING_IDLE;
uint32_t bondingStarttime = 0;
int8_t candidateCompleted = -1;

struct bondingRequest_t {
  uint32_t node;
  uint32_t startt;
};

SimpleList<bondingRequest_t> bondingCandidates;
bondingRequest_t bondingCandidate;

void setup()
{  
  Serial.begin(115200);
  delay(25);

  // Start Display for Feedback first
  initScreen();

  //check for battery to not discharge too much
  checkBatteryCharge();

  //check filesystem
  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS initialisation failed!");
    while (1)
      yield(); // Stay here twiddling thumbs waiting
  }
  Serial.print("SPIFFS initialised.\r\n");

  // Perform tasks necessary on a fresh start
  if (freshStart)
  {
    //load configuration from and state from persistent storage
    //calibrarte button threshold
    touchInput.calibrate();

    freshStart = false;
  }

  // Start up mesh connection and callbacks
  mesh.setDebugMsgTypes(ERROR | DEBUG); // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  // Setup user input sensing
  touchInput.begin();
  touchInput.setBtnHandlers(buttonHandler, onPressed);

  touchInput._buttonLeft.onPressedFor(BTNHOLDDELAY, setTempo);
  touchInput._buttonRight.onPressedFor(BTNHOLDDELAY, userStartBonding);

  userScheduler.addTask(taskCheckButtonPress);
  taskCheckButtonPress.enable();

  hwbutton1.begin();
  hwbutton2.begin();
  hwbutton1.onPressed(pressedShutdown);
  hwbutton2.onPressed(checkDeviceStatus);

  //add tasks for later use to scheduler
  userScheduler.addTask(taskCheckBattery);
  taskCheckBattery.enableDelayed(BATTERY_CHARGE_CHECK_INTERVAL);
  userScheduler.addTask(taskSendBPM);
  userScheduler.addTask(taskReconnectMesh);
  userScheduler.addTask(taskBondingPing);

  //Output
  userScheduler.addTask(taskVisualiser);
  taskVisualiser.enable();

  userScheduler.addTask(taskShowLogo);
  setWallpapers(mesh.getNodeId());
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
  float voltage = getInputVoltage();
  if (voltage < 3.0)
  {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Got no juice :(", tft.width() / 2, tft.height() / 2);
    goToSleep();
  } else if (voltage > 4.5)
  {
    Serial.println("Charging");
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

void turnOffWifi() {
  turnOffWifi(5000);
}

void turnOffWifi(uint32_t reconnectDelay)
{
  mesh.stop();
  Serial.println("Disconnected from mesh! Wait for 5 seconds to reconnect.");
  taskReconnectMesh.setCallback([]() {
    Serial.println("Trying to reconnect to mesh!");
    mesh.initStation();
  });
  userScheduler.addTask(taskReconnectMesh);
  taskReconnectMesh.restartDelayed(reconnectDelay);
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
  if (currentState != STATE_TAPTEMPO) { //replace thsi with a propoer check in the state machine
    touchInput.pressed();
  }
}

void checkButtonPress()
{
  touchInput.tick();
  hwbutton1.read();
  hwbutton2.read();
  if (currentState == STATE_TAPTEMPO) {
    visualiser.tapTempo.update(touchInput._buttonLeft.isPressed());
  }
  else if (currentState == STATE_BONDING && touchInput._buttonRight.isReleased())
  {
    userAbortBonding();
  }
}

void checkDeviceStatus()
{
  // displayMessage("Status check");
  showVoltage();

  // Send the current touch values over the mesh to remotely debug
  String msg = String(touchRead(TOUCHPIN_LEFT)) + " : " + String(touchRead(TOUCHPIN_RIGHT));
  mesh.sendBroadcast(msg);

  // Check the different states of the buttons
  String bs = String(touchInput._buttonLeft.isPressed()) + " : " +
              String(touchInput._buttonLeft.isReleased()) + " : " +
              String(touchInput._buttonLeft.pressedFor(500)) + " : " +
              String(touchInput._buttonLeft.wasPressed()) + " : " +
              String(touchInput._buttonLeft.wasReleased());
  Serial.println("Button states: " + bs);

  taskShowLogo.restartDelayed();
}

void setTempo()
{
  //Tell that taps are registered
  displayMessage("<-- Tap Tempo!");

  //turn of other button functions
  currentState = STATE_TAPTEMPO;

  //TODO: give feddback for each tap

  //sendBPM after
  taskSendBPM.setCallback([]() {
    mesh.sendBroadcast("BPM" + String(visualiser.tapTempo.getBPM()));
    // Serial.println("Sending BPM" + String(visualiser.tapTempo.getBPM()));
    currentState = STATE_IDLE;
    showHomescreen();
  });
  taskSendBPM.restartDelayed();
}


void buttonHandler(uint8_t keyCode)
{
  Serial.println("Tap " + String(keyCode));

  static bool firsttime = true;
  if (firsttime) {
    firsttime = !firsttime;
    return;
  }

  if (currentState == STATE_IDLE)
  {
    if (keyCode == TAP_LEFT)
    {
      visualiser.nextPattern();
    }
    else if (keyCode == TAP_RIGHT)
    {
      nextWallpaper();
    }
  }
}

/*
* HANDLE BONDING (check when the pin is active and when not / end seems sus)
*/

void initiateBonding() {
  currentState = STATE_BONDING;
  bondingState = BONDING_REQUESTED;
  candidateCompleted = -1;

  //visuliser blink one LED
  displayMessage("Searching Peer ...");
  taskShowLogo.disable();

  taskBondingPing.setIterations(TASK_FOREVER);
  taskBondingPing.enable();

  bondingCandidates.remove_if([](bondingRequest_t c) { return mesh.getNodeTime() - c.startt > BONDINGTIMEOUT * 1000; }); // clean out old requests
  if (bondingCandidates.empty())
  {
    Serial.println("No Valid Requests stored");
  }
  else // bondingRequest was received earlier
  {
    bondingCandidate = bondingCandidates.front();
    initiateBondingHandShake();
  }
}

void initiateBondingHandShake() {
  bondingState = BONDING_STARTED;
  bondingStarttime = mesh.getNodeTime();
  taskBondingPing.enable();
  Serial.printf("Bonding requested to %u\r\n", bondingCandidate.node);
}

void initiateBondingSequence() {
  bondingState = BONDING_INPROGRESS;
  taskBondingPing.enable();
  displayMessage("Bonding...");
  visualiser.fillMeter((bondingCandidate.startt+bondingStarttime) / 2000, HANDSHAKETIME, CRGB::Blue); // fill meter
  Serial.printf("Bonding now with %u\r\n", bondingCandidate.node);
}

void abortBondingSequence() {
  bondingState = BONDING_IDLE;
  visualiser.blink(300, 3, CRGB::Red); //this one is called 
}

void completeBondingSequence()
{
  currentState = STATE_IDLE;
  bondingState = BONDING_IDLE;
  
  //write into storage
  addWallpaperIndex(candidateCompleted);

  visualiser.blink(500, 3, CRGB::Green); // fill meter
  displayMessage("Bonding Complete!");
  taskShowLogo.restartDelayed();
}

void userStartBonding()
{
  if (!nodes.empty())
  {
    initiateBonding();
  }
  else
  {
    displayMessage("No one around...");
    taskShowLogo.restartDelayed();
    visualiser.blink(300, 3, CRGB::Red);
    currentState = STATE_IDLE;
  }
}

void userAbortBonding()
{
  currentState = STATE_IDLE;
  sendMessage(BP_BONDING_REQUEST_ONE);
  taskBondingPing.disable();
  candidateCompleted = -1;
  if (bondingState == BONDING_INPROGRESS || bondingState == BONDING_COMPLETE)
    bondingCandidates.remove_if([](bondingRequest_t c) { return c.node == bondingCandidate.node; });
  abortBondingSequence();
  displayMessage("Abort");
  taskShowLogo.restartDelayed();
  Serial.println("Bonding aborted by user" + String(currentState));
}

void userFinishBonding()
{
  bondingState = BONDING_COMPLETE;
  Serial.printf("Bonding completed by user, sending to %u/n", bondingCandidate.node);
  taskBondingPing.enable();
  taskBondingPing.setIterations(5); //remove magic number
  if (candidateCompleted > -1) {
    completeBondingSequence();
  }
}

void sendBondingPing() {
  Serial.printf("Ping:BondingState:%u:%u\r\n", bondingState, taskBondingPing.getRunCounter());
  switch (bondingState)
  {
  case BONDING_REQUESTED:
    sendMessage(BP_BONDING_REQUEST_ALL);
    break;
  case BONDING_INPROGRESS:
    if (taskBondingPing.getRunCounter() > HANDSHAKETIME / BONDINGPING) {
      userFinishBonding();
    }
  case BONDING_STARTED:
    mesh.sendSingle(bondingCandidate.node, BP_BONDING_START + String(bondingStarttime)); // send bonding handshake
    break;
  case BONDING_COMPLETE:
    if (taskBondingPing.isLastIteration())
    {
      abortBondingSequence(); // reset to being availible for boding if boding goes on
      initiateBonding();
    }
    else mesh.sendSingle(bondingCandidate.node, BP_BONDING_COMPLETE + String(getCurrentWallpaperIndex()));
    break;

  default:
    //IDLE do nothing
    break;
  }
}

void handleBondingRequests(uint32_t from, String &msg)
{
  if (msg.startsWith(BP_BONDING_REQUEST_ALL)) {
    bondingCandidates.remove_if([](bondingRequest_t c) { return mesh.getNodeTime() - c.startt > BONDINGTIMEOUT * 1000; });
    // Add new request to the cue
    bondingRequest_t newCandidate;
    newCandidate.node = from;
    newCandidate.startt = mesh.getNodeTime();

    if (bondingState == BONDING_REQUESTED) { //user had requested before, so start bonding
      bondingCandidate = newCandidate;
      bondingCandidates.push_front(newCandidate);
      initiateBondingHandShake();
    }
    else //save for later
    {
      bondingCandidates.push_back(newCandidate);
      if (bondingState == BONDING_IDLE)
        Serial.println("User has not initiated Bonding yet!");
      else
        Serial.println("Bonding already started/In Progress.");
    }
  }
  else if (msg.startsWith(BP_BONDING_REQUEST_ONE)) { // Abort ercevied
    if ((bondingState == BONDING_STARTED || bondingState == BONDING_INPROGRESS || bondingState == BONDING_COMPLETE) && bondingCandidate.node == from)
    { // abort current bonding procedure
      Serial.println("Bonding aborted by peer");
      abortBondingSequence();
      initiateBonding();
    }
    bondingCandidates.remove_if([&from](bondingRequest_t c) { return c.node == from; });
  }
  else if (msg.startsWith(BP_BONDING_START))
  {
    uint32_t candidateTime = msg.substring(4).toInt();
    if (bondingState == BONDING_STARTED && bondingCandidate.node == from) // we started first and now
    {
      bondingCandidate.startt = candidateTime;
      initiateBondingSequence();
    }
    else if (bondingState == BONDING_REQUESTED) // if bonding hasnt strted but was requested, skip start and bond immediatly
    {
      bondingCandidate.node = from;
      bondingCandidate.startt = candidateTime;
      bondingCandidates.push_front(bondingCandidate);
      initiateBondingHandShake();
      initiateBondingSequence();
    }
    else if (bondingState == BONDING_IDLE) // this is faulty / unecessary
    {
      bondingCandidate.node = from;
      bondingCandidate.startt = candidateTime;
      bondingCandidates.push_front(bondingCandidate);
      // user did mno start it, so discard this? Or shouled we store it? 
      Serial.println("User hasn't started bonding or is in progress with other peer, so we do not react");
    }
  }
  else if (msg.startsWith(BP_BONDING_COMPLETE))
  {
    if (from == bondingCandidate.node) {
      candidateCompleted = msg.substring(4).toInt();
      Serial.println("Bonding completed by peer. Received picture: " + String(candidateCompleted));
      if (bondingState == BONDING_COMPLETE) {
        completeBondingSequence();
      }
    }
    else {
      Serial.println("Bonding completed by non-peer");
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

  Serial.printf("Sending message: %s\r\n", msg.c_str());

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
  Serial.printf("Received from %u msg=%s\r\n", from, msg.c_str());
  Serial.println("");

  String protocol = msg.substring(0,4);
  if (protocol == BP_BONDING_REQUEST_ALL || protocol == BP_BONDING_REQUEST_ONE || protocol == BP_BONDING_START || protocol == BP_BONDING_COMPLETE)
  {
    handleBondingRequests(from, msg);
  }
  else if (msg.startsWith(BP_NEWBPM))
  {
    float bpm = msg.substring(3).toFloat();
    // Serial.println("Recevied BPM: " + String(bpm));
    visualiser.tapTempo.setBPM(bpm);
  }
  else if (msg.startsWith("Anythgin else"))
  {
    //do anything else based on network
  }
}

void newConnectionCallback(uint32_t nodeId)
{
  // Display change on tft

  Serial.printf("New Connection, nodeId = %u\r\n", nodeId);
  // Serial.printf("--> startHere: New Connection, %s\r\n", mesh.subConnectionJson(true).c_str());
  // Serial.println("");
  updateNumNodes(nodes.size());
  showHomescreen();
}

void changedConnectionCallback()
{
  Serial.printf("Changed connections\r\n");
  // Display changes on tft

  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\r\n", nodes.size());
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
  Serial.printf("Adjusted time %u. Offset = %d\r\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay)
{
  Serial.printf("Delay to node %u is %d us\r\n", from, delay);
}
