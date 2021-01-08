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
#include "customProtocol.hpp"
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
bool receivedInvitationCallback(protocol::Variant variant);
bool receivedExchangeCallback(protocol::Variant variant);
bool receivedAbortCallback(protocol::Variant variant);

FileStorage fileStorage{};
RTC_DATA_ATTR BadgeConfig configuration = {NUM_PICS, {0, 1, 2}}; // keep configuration in deep sleep

EasyButton hwbutton1(HW_BUTTON_PIN1);
EasyButton hwbutton2(HW_BUTTON_PIN2);
CapacitiveKeyboard touchInput(TOUCHPIN_LEFT, TOUCHPIN_RIGHT, TTHRESHOLD, TTHRESHOLD, DEBOUNCE_TIME);
RTC_DATA_ATTR bool freshStart = true;

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
TFT_eSPI tft(TFT_WIDTH, TFT_HEIGHT); // Invoke custom TFT library

// Task variables
Task taskCheckBattery(BATTERY_CHARGE_CHECK_INTERVAL, TASK_FOREVER, &checkBatteryCharge);
Task taskCheckButtonPress(TASK_CHECK_BUTTON_PRESS_INTERVAL, TASK_FOREVER, &checkButtonPress);
Task taskVisualiser(VISUALISATION_UPDATE_INTERVAL, TASK_FOREVER, &showVisualisations);
Task taskShowLogo(LOGO_DELAY, TASK_ONCE, &showHomescreen);
Task taskBondingPing(BONDINGPING, TASK_FOREVER, &sendBondingPing);
Task taskSendBPM(TAPTIME,TASK_ONCE);
Task taskReconnectMesh(TAPTIME, TASK_ONCE);

enum appState_t
{
  STATE_IDLE,
  STATE_BONDING,
  STATE_SCORE,
  STATE_PROXIMITY,
  STATE_GROUP,
  STATE_TAPTEMPO
};
appState_t currentState = STATE_IDLE;

enum exchangeState_t
{
  BONDING_REQUESTED,
  BONDING_STARTED,
  BONDING_INPROGRESS,
  BONDING_COMPLETE
};
exchangeState_t bondingState = BONDING_REQUESTED;
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

  // Start up mesh connection and callbacks
  mesh.setDebugMsgTypes(ERROR | DEBUG); // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);
  mesh.onPackage(INVITATION_PKG, &receivedInvitationCallback);
  mesh.onPackage(EXCHANGE_PKG, &receivedExchangeCallback);
  mesh.onPackage(ABORT_PKG, &receivedAbortCallback);

  // Perform tasks necessary on a fresh start
  if (freshStart)
  {
    // load configuration from and state from persistent storage
    if (!fileStorage.loadConfiguration(configuration)) 
    {
      badges_t badges[NUM_BADGES] = {
          {2884960141, 0, {3, 4, 5}},
          {3519576873, 1, {0, 1, 2}}};
      uint32_t nodeid = mesh.getNodeId();
      for (size_t i = 0; i < NUM_BADGES; i++)
      {
        if (badges[i].node == nodeid)
        {
          memcpy(configuration.pics, badges[i].Pics, sizeof(badges[i].Pics));
          configuration.numPics = NUM_PICS;
          break;
        }
      }
      
      fileStorage.saveConfiguration(configuration);

      // fileStorage.printFile(CONFIG_FILE);
    }

    freshStart = false;
  }

  // Setup user input sensing
  touchInput.calibrate();
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
  static bool charging = false;
  float voltage = getInputVoltage();
  if (voltage < 3.0)
  {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Got no juice :(", tft.width() / 2, tft.height() / 2);
    goToSleep();
  } else if ((!charging && voltage >= 4.5) || (charging && voltage < 4.5))
  {
    if (charging)
      Serial.println("Stopped Charging");
    else
      Serial.println("Started Charging");
    displayMessage("Just a sec...");
    touchInput.calibrate();
    charging = !charging;
    showHomescreen();
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
      //Should become change the visualiser state -> Off / Score / Prox / Group
      //But how to sweitch animation styles?
    }
    else if (keyCode == TAP_RIGHT)
    {
      nextPicture();
    }
  }
}

/*
* HANDLE BONDING (check when the pin is active and when not / end seems sus)
*/

void initiateBonding()
{
  Serial.println("Started initiateBonding");
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

void initiateBondingHandShake()
{
  bondingState = BONDING_STARTED;
  bondingStarttime = mesh.getNodeTime();
  taskBondingPing.enable();
  Serial.printf("initiateBondingHandshake to %u\r\n", bondingCandidate.node);
}

void initiateBondingSequence()
{
  Serial.printf("initiateBondingSequence with %u\r\n", bondingCandidate.node);
  bondingState = BONDING_INPROGRESS;
  taskBondingPing.enable();
  displayMessage("Bonding...");
  visualiser.fillMeter((bondingCandidate.startt+bondingStarttime) / 2000, HANDSHAKETIME, CRGB::Blue); // fill meter
}

void abortBondingSequence() {
  visualiser.blink(300, 3, CRGB::Red); //this one is called 
}

void completeBondingSequence()
{
  currentState = STATE_IDLE;
  if (candidateCompleted == -1)
  {
    Serial.println("No candidated picture but bonding completion was called!");
    return;
  }
  Serial.println("Started complete bonding seq");

  //write into configuration and storage
  uint8_t *end = configuration.pics + configuration.numPics;
  uint8_t *currentPic = std::find(std::begin(configuration.pics), end, candidateCompleted);
  if (currentPic == end)
  {
    Serial.println("it is a new pic, so we add it and store the config");
    configuration.pics[configuration.numPics] = candidateCompleted; //fitler for duplicates before storing
    configuration.numPics++;
    fileStorage.saveConfiguration(configuration);
    fileStorage.printFile(CONFIG_FILE);
  }

  Serial.println("Set current picture");
  Serial.println(std::distance(configuration.pics, currentPic));
  setCurrentPicture(std::distance(configuration.pics, currentPic));

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
  if (bondingState > BONDING_REQUESTED) {
    auto pkg = AbortPackage(mesh.getNodeId(), bondingCandidate.node);
    mesh.sendPackage(&pkg);
  }
  taskBondingPing.disable();
  candidateCompleted = -1;
  if (bondingState == BONDING_INPROGRESS || bondingState == BONDING_COMPLETE)
    bondingCandidates.remove_if([](bondingRequest_t c) { return c.node == bondingCandidate.node; });
  abortBondingSequence();
  displayMessage("Abort");
  taskShowLogo.restartDelayed();
  Serial.println("Bonding aborted by user : state " + String(currentState));
}

void userFinishBonding()
{
  bondingState = BONDING_COMPLETE;
  Serial.printf("Bonding completed by user, sending to %u \r\n", bondingCandidate.node);
  taskBondingPing.setIterations(5); //remove magic number
  taskBondingPing.enable();
  if (candidateCompleted > -1) {
    completeBondingSequence();
  }
}

void sendBondingPing() {
  Serial.printf("Ping:BondingState:%u:%u\r\n", bondingState, taskBondingPing.getRunCounter());

  if (bondingState == BONDING_REQUESTED) {
    auto pkg = InvitationPackage(mesh.getNodeId());
    mesh.sendPackage(&pkg);
  } else { // BONDING IN PROGRESS
    auto pkg = ExchangePackage(mesh.getNodeId(), bondingCandidate.node, getCurrentPicture());
    switch (bondingState)
    {
    case BONDING_INPROGRESS:
      if (taskBondingPing.getRunCounter() > HANDSHAKETIME / BONDINGPING) {
        userFinishBonding();
      }
    case BONDING_STARTED:
      pkg.starttime = bondingStarttime;
      pkg.progress = ExchangePackage::PROGRESS_START;
      mesh.sendPackage(&pkg);
      
      break;
    case BONDING_COMPLETE:
      pkg.progress = ExchangePackage::PROGRESS_COMPLETE;
      mesh.sendPackage(&pkg);
      break;

    default:
      //IDLE do nothing
      break;
    }

  }
}

bool receivedInvitationCallback(protocol::Variant variant)
{
  auto pkg = variant.to<InvitationPackage>();

  Serial.printf("Received InvitationPackage from %u\r\n", pkg.from);

  bondingCandidates.remove_if([](bondingRequest_t c) { return mesh.getNodeTime() - c.startt > BONDINGTIMEOUT * 1000; });
  // Add new request to the cue
  bondingRequest_t newCandidate;
  newCandidate.node = pkg.from;
  newCandidate.startt = mesh.getNodeTime();

  if (currentState != STATE_BONDING) {
      Serial.println("User has not initiated Bonding yet!");
  } else if (bondingState == BONDING_REQUESTED)
  { //user had requested before, so start bonding
    bondingCandidate = newCandidate;
    bondingCandidates.push_front(newCandidate);
    initiateBondingHandShake();
  } else //save for later
  {
    bondingCandidates.push_back(newCandidate);
    Serial.println("Bonding already started/In Progress.");
  }
  return true;
}

bool receivedExchangeCallback(protocol::Variant variant)
{
  auto pkg = variant.to<ExchangePackage>();

  Serial.printf("Received ExchangePackage %u from %u\r\n", pkg.progress, pkg.from);

  if (currentState == STATE_IDLE) {
    Serial.println("Bonding not active, so package is discarded");
    return true;
  }

  if (pkg.progress == ExchangePackage::PROGRESS_START)
  {
    if (bondingState == BONDING_STARTED && bondingCandidate.node == pkg.from) // we started first and now
    {
      bondingCandidate.startt = pkg.starttime;
      initiateBondingSequence();
    }
    else if (bondingState == BONDING_REQUESTED) // if bonding hasnt strted but was requested, skip start and bond immediatly
    {
      bondingCandidate.node = pkg.from;
      bondingCandidate.startt = pkg.starttime;
      bondingCandidates.push_front(bondingCandidate);
      initiateBondingHandShake();
      initiateBondingSequence();
    }
  }
  else if (pkg.progress == ExchangePackage::PROGRESS_COMPLETE)
  {
    if (pkg.from == bondingCandidate.node)
    {
      candidateCompleted = pkg.picture;
      Serial.println("Bonding completed by peer. Received picture: " + String(candidateCompleted));
      if (bondingState == BONDING_COMPLETE)
      {
        completeBondingSequence();
      }
    }
    else
    {
      Serial.println("Bonding completed by non-peer");
    }
  }
  return true;
}

bool receivedAbortCallback(protocol::Variant variant) { // Abort ercevied
  auto pkg = variant.to<AbortPackage>();

  Serial.printf("Received AbortPackage from %u\r\n", pkg.from);

  if ((bondingState == BONDING_STARTED || bondingState == BONDING_INPROGRESS || bondingState == BONDING_COMPLETE) && bondingCandidate.node == pkg.from)
  { // abort current bonding procedure
    Serial.println("Bonding aborted by peer");
    abortBondingSequence();
    initiateBonding();
  }
  bondingCandidates.remove_if([&pkg](bondingRequest_t c) { return c.node == pkg.from; });
  return true;
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

  if (msg.startsWith(BP_NEWBPM))
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
  if(nodes.size() > 0)
  {
    //check nodes for group adn set accordingly
    visualiser.setProximityStatus(PROXIMITY_NEARBY);
  } else
  {
    visualiser.setProximityStatus(PROXIMITY_ALONE);
  }
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
