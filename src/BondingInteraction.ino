//************************************************************
// This application uses the painless Mesh library for connectivity and FastLED library to control the NeoPixel LEDs
//
// Felix A. Epp <felix.epp@aalto.fi>
//
//************************************************************
#define ARDUINOJSON_USE_LONG_LONG 1
#include <painlessMesh.h>
#include "BadgeProtocol.hpp"
#include "TouchButtons.h"
#include "StatusVisualiser.h"
#include "ScreenController.h"
#include "time.h"
#include <sys/time.h>
#include "esp_adc_cal.h"

// Prototypes
void routineCheck();
void setTempo();
void checkDeviceStatus();
void buttonHandler(TouchButtons::InputType keyCode);
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
bool receivedBeatCallback(protocol::Variant variant);
bool receivedTimeCallback(protocol::Variant variant);

FileStorage fileStorage{};
RTC_DATA_ATTR badgeConfig_t configuration = {NUM_PICS, {}, 0xffffff, {0, 1, 2}}; // keep configuration in deep sleep

EasyButton hwbutton1(HW_BUTTON_PIN1);
EasyButton hwbutton2(HW_BUTTON_PIN2);
TouchButtons touchInput(TOUCHPIN_LEFT, TOUCHPIN_RIGHT, TTHRESHOLD, TTHRESHOLD, DEBOUNCE_TIME);
RTC_DATA_ATTR bool freshStart = true;

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

int vref = 1100;
int32_t energyCountdown = ENERGY_SAFE_TIMEOUT;
bool calc_delay = false;
SimpleList<uint32_t> nodes;
SimpleList<uint32_t> groupNodes;

// @Override This function is called by FastLED inside lib8tion.h.Requests it to use mesg.getNodeTime instead of internal millis() timer.
uint32_t get_millisecond_timer_hook()
{
  return mesh.getNodeTime() / 1000;
}
StatusVisualiser visualiser(get_millisecond_timer_hook, 64);
TFT_eSPI tft(TFT_WIDTH, TFT_HEIGHT); // Invoke custom TFT library

// Task variables
Task taskCheckBattery(BATTERY_CHARGE_CHECK_INTERVAL, TASK_FOREVER, &routineCheck);
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
  STATE_TAPTEMPO,
  STATE_WAITFORINTERACTION
};
appState_t currentState = STATE_WAITFORINTERACTION;

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

  // Start Display for feedback first
  initScreen();
  
  //check for battery to not discharge too much
  checkBatteryCharge(true);

  //Start LEDs 
  visualiser.fillAll(CRGB::Green);
  displayMessage(F("Starting Up..."));

  //check filesystem
  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS initialisation failed!");
    while (1)
      yield(); // Stay here twiddling thumbs waiting
  }
  Serial.print("SPIFFS initialised.\r\n");

  // Start up mesh connection
  mesh.setDebugMsgTypes(ERROR | DEBUG); // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler);
  mesh.setContainsRoot(true);

  // Perform tasks necessary on a fresh start: load configurations
  if (freshStart)
  {
    Serial.println(F("Fresh start, load configuration"));

    if (!fileStorage.loadConfiguration(configuration)) 
    {
      // load configuration from persistent storage
      fileStorage.initConfiguration(configuration, mesh.getNodeId());
    }
    // fileStorage.printFile(CONFIG_FILE);
    freshStart = false;
  }
  Serial.printf("Booting with the following configurations: \r\n - colour: %#08x\r\n - pictures: %u\r\n", configuration.color, configuration.numPics);
  // GROUP NODES FROM CONFIG FILE=??=?=
  for (size_t i = 0; i < MAX_GROUP_SIZE && configuration.group[i] != 0; i++)
  {
    groupNodes.push_back(configuration.group[i]);
    Serial.printf("Adding %u to group\r\n", configuration.group[i]);
  }

  // Setup all network listeners
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);
  mesh.onPackage(INVITATION_PKG, &receivedInvitationCallback);
  mesh.onPackage(EXCHANGE_PKG, &receivedExchangeCallback);
  mesh.onPackage(ABORT_PKG, &receivedAbortCallback);
  mesh.onPackage(BEAT_PKG, &receivedBeatCallback);
  mesh.onPackage(DATE_PKG, &receivedTimeCallback);

  // Setup user input sensing (Do we need to wait here until people do not touch anymore???)
  touchInput.calibrate();
  touchInput.begin();
  touchInput.setBtnHandlers(buttonHandler, onPressed, onPressedFor);
  userScheduler.addTask(taskCheckButtonPress);
  taskCheckButtonPress.enable();

  hwbutton1.begin();
  hwbutton2.begin();
  hwbutton1.onPressed(pressedShutdown);
  hwbutton2.onPressed(checkDeviceStatus);
  hwbutton2.onSequence(2, 1000, printLog);

  //add tasks for later use to scheduler
  userScheduler.addTask(taskCheckBattery);
  taskCheckBattery.enableDelayed(BATTERY_CHARGE_CHECK_INTERVAL);
  userScheduler.addTask(taskSendBPM);
  userScheduler.addTask(taskReconnectMesh);
  userScheduler.addTask(taskBondingPing);

  visualiser.setDefaultColor(configuration.color);
  userScheduler.addTask(taskVisualiser);
  visualiser.turnOff();
  userScheduler.addTask(taskShowLogo);
  displayMessage(F("Filled the survey?"));

  fileStorage.logBootEvent(mesh.getNodeTime());

  randomSeed(analogRead(A0));
}

void loop()
{
  mesh.update();
}

/*
* POWER MANAGEMENT
*/

void logTime()
{
  time_t now;
  struct tm timeinfo;

  time(&now);

  localtime_r(&now, &timeinfo);
  Serial.print(now);
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void wakeup_callback()
{
  //placeholder callback function
}

void routineCheck() {
  checkBatteryCharge(false);
}

// Check if on Battery and empty, if so go to sleep to protect boot loop on low voltage
void checkBatteryCharge(bool boot)
{
  if (boot) {
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars); //Check type of calibration value used to characterize ADC
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
      Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
      vref = adc_chars.vref;
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
      Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
    }
    else
    {
      Serial.println("Default Vref: 1100mV");
    }
  }

  energyCountdown -= BATTERY_CHARGE_CHECK_INTERVAL / 1000;
  if (energyCountdown*1000 < BATTERY_CHARGE_CHECK_INTERVAL)
  {
    if (nodes.size() == 0)
    {
      goToSleep(true);
    }
  }

  static bool charging = false;
  float voltage = getInputVoltage();
  if (voltage < 3.0)
  {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Got no juice :(", tft.width() / 2, tft.height() / 2);
    goToSleep(true);
  }
  else if ((!charging && voltage >= 4.5) || (charging && voltage < 4.5))
  {
    charging = !charging;
    if (charging)
      Serial.println("Started Charging");
    else
      Serial.println("Stopped Charging");
    if (!boot && currentState == STATE_IDLE)
    {
      displayMessage(F("Calibrating touch..."));
      touchInput.calibrate();
      showHomescreen();
    }
  }

  if (calc_delay && !boot)
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
  pressedShutdown(false);
}

void pressedShutdown(bool touch)
{
  displayMessage("Shutting down");
  goToSleep(touch);
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
void goToSleep(bool touch)
{
  visualiser.turnOff();
  mesh.stop();
  Serial.println("Disconnected from mesh!");
  if (touch)
  {
    touchAttachInterrupt(TOUCHPIN_LEFT, wakeup_callback, STHRESHOLD);
    touchAttachInterrupt(TOUCHPIN_RIGHT, wakeup_callback, STHRESHOLD);
    esp_sleep_enable_touchpad_wakeup();
  }
  else
  {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)HW_BUTTON_PIN1, 0);
  }
  Serial.println("Goind to sleep in 2 sec!");
  delay(2000);
  esp_deep_sleep_start();
}

/*
* USER INPUT
*/

void onPressedFor()
{
  touchInput.pressedFor();
}

void onPressed()
{
  touchInput.pressed();
}

void checkButtonPress()
{
  touchInput.tick();
  hwbutton1.read();
  hwbutton2.read();
  if (currentState == STATE_TAPTEMPO)
    visualiser.updateBeat(touchInput._buttonLeft.isPressed());
  else {
    if (currentState == STATE_BONDING && bondingState != BONDING_COMPLETE && touchInput._buttonRight.isReleased())
    {
      userAbortBonding();
    }
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

  // Open file for reading
  fs::File file = SPIFFS.open(LOG_FILE);
  if (!file)
  {
    Serial.println(F("Failed to read file"));
    return;
  }
  Serial.print("File size: " + String(file.size()) + "\r\n");
  file.close();

  Serial.print("Storage used: " + String(SPIFFS.usedBytes()) + "/" + String(SPIFFS.totalBytes()) + "\r\n");

  taskShowLogo.restartDelayed();
}

void printLog()
{
  Serial.println("LOGSTART" + String(mesh.getNodeId()));
  fileStorage.printFile(LOG_FILE);
  Serial.println("LOGEND");
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
    auto pkg = BeatPackage(mesh.getNodeId(), visualiser.getBeatLength());
    mesh.sendPackage(&pkg);
    currentState = STATE_IDLE;
    fileStorage.logBeatEvent(mesh.getNodeTime(), visualiser.getBeatLength(), mesh.getNodeId());
    showHomescreen();
  });
  taskSendBPM.restartDelayed();
}


void buttonHandler(TouchButtons::InputType keyCode)
{
  Serial.println("Tap " + String(keyCode));
  energyCountdown = ENERGY_SAFE_TIMEOUT;

  if (keyCode == TouchButtons::HOLD_BOTH)
  {
    pressedShutdown(true);
  }
  else if (currentState == STATE_WAITFORINTERACTION) {
    if (keyCode != TouchButtons::NO_TAP) {
      currentState = STATE_IDLE;
      showHomescreen();
      visualiser.startPattern();
      taskVisualiser.enable();
    }
  }
  else if (currentState == STATE_IDLE)
  {
    if (keyCode == TouchButtons::TAP_LEFT)
    {
      visualiser.nextPattern();
    }
    else if (keyCode == TouchButtons::TAP_RIGHT)
    {
      nextPicture();
      fileStorage.logPictureEvent(mesh.getNodeTime(), getCurrentPicture());
    }
    else if (keyCode == TouchButtons::HOLD_LEFT)
    {
      setTempo();
    }
    else if (keyCode == TouchButtons::HOLD_RIGHT)
    {
      userStartBonding();
    }
  }
  else if (currentState == STATE_TAPTEMPO)
  {
    if (keyCode == TouchButtons::HOLD_LEFT) 
    {
      taskSendBPM.disable();
      currentState = STATE_IDLE;
      showHomescreen();
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

  fileStorage.logSharingEvent(mesh.getNodeTime(), bondingCandidate.node, candidateCompleted);

  Serial.println("Set current picture");
  setCurrentPicture(std::distance(configuration.pics, currentPic));
  fileStorage.logPictureEvent(mesh.getNodeTime(), getCurrentPicture());


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
  Serial.printf("Ping:BondingState:%u:%lu\r\n", bondingState, taskBondingPing.getRunCounter());

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

  if (currentState == STATE_BONDING 
      && bondingState != BONDING_REQUESTED
      && bondingCandidate.node == pkg.from)
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

bool receivedTimeCallback(protocol::Variant variant)
{
  auto pkg = variant.to<DateTimePackage>();

  Serial.printf("Received DateTime %lu from %u\r\n", pkg.datetime, pkg.from);
  struct timeval incoming_time;
  incoming_time.tv_sec = pkg.datetime;
  incoming_time.tv_usec = abs(mesh.getNodeTime()-pkg.nodetime);
  settimeofday(&incoming_time, NULL);

  logTime();
  return true;
}

bool receivedBeatCallback(protocol::Variant variant)
{
  auto pkg = variant.to<BeatPackage>();

  Serial.printf("Received BPM %ld from %u\r\n", pkg.beatLength, pkg.from);
  fileStorage.logBeatEvent(mesh.getNodeTime(), pkg.beatLength, pkg.from);
  visualiser.setBeatLength(pkg.beatLength);
  return true;
}


/* Broadcast a message to all nodes*/
void sendMessage(String msg)
{
  Serial.printf("Sending message: %s\r\n", msg.c_str());

  mesh.sendBroadcast(msg);
}

/* Controller for incoming messages that prints all incoming messages and listens on touch and bonding events */
void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Received from %u msg=%s\r\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId)
{
  // Display change on tft

  Serial.printf("New Connection, nodeId = %u\r\n", nodeId);
  // Serial.printf("--> startHere: New Connection, %s\r\n", mesh.subConnectionJson(true).c_str());
  // Serial.println("");
  updateNumNodes(nodes.size());
  if (currentState == STATE_IDLE) showHomescreen();
}

// Called when a change in the connections is registered and control animations based on the change
void changedConnectionCallback()
{
  Serial.printf("Changed connections\r\n");

  nodes = mesh.getNodeList();

  fileStorage.logConnectionEvent(mesh.getNodeTime(), nodes);

  updateNumNodes(nodes.size());
  if(nodes.size() > 0)
  {
    calc_delay = true;

    // Cycle through all connected nodes and check if any outside the group are present
    bool groupMatch = true;
    SimpleList<uint32_t>::const_iterator node = nodes.begin();
    while ( node != nodes.end() && groupMatch) {
      Serial.println(*node);
      groupMatch = (*std::find(groupNodes.begin(), groupNodes.end(), *node) == *node);
      node++;
    }
    if (groupMatch)
    {
      visualiser.setProximityStatus(PROXIMITY_NEARBY);
    }
    else
    {
      visualiser.setProximityStatus(PROXIMITY_GROUP);
    }
  }
  else
  {
    visualiser.setProximityStatus(PROXIMITY_ALONE);
  }
  if (currentState == STATE_IDLE) showHomescreen();
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\r\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay)
{
  Serial.printf("Delay to node %u is %d us\r\n", from, delay);
}
