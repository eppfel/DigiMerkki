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
void sendCypher();
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
#define STATE_CYPHER 1
uint8_t currentState = STATE_IDLE;

uint16_t cypher;
uint8_t lastKey;
uint16_t cypherPeer;
uint32_t cypherNode;
uint32_t bondingStarttime = 0;
uint32_t bondingRequestNode;
uint32_t bondingSuccessNode;

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
  case STATE_CYPHER:
    // typeCypher(keyCode);
    break;
  default: //idle

    Serial.println("Tap " + String(keyCode));
    if (keyCode == HOLD_BOTH)
    {
      currentState = STATE_CYPHER;
      cypher = 0;
      Serial.println("Switch from idle to cypher-input");
      taskShowLogo.disable();
      tft.fillScreen(TFT_BLACK);
      tft.setTextDatum(ML_DATUM);
      tft.drawString("Cypher: ", 0, tft.height() / 2);
      visualiser.blink(200, 3, CRGB::HotPink, StatusVisualiser::STATE_METER);
    }
    else if (keyCode == TAP_LEFT)
    {
      visualiser.nextPattern();
    }
    else if (keyCode == TAP_RIGHT)
    {
      nextWallpaper();
    } else if (keyCode == TAP_BOTH) {
      displayMessage("Tappd both");
    }
    break;
  }
}

String cypherString(uint16_t cypher)
{
  String cypherword = "";
  while (cypher >= 1)
  {

    // Serial.println("in the loop");
    switch (cypher & 7)
    {
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

void sendCypher()
{

  String msg = "Cypher : ";
  msg += String(cypher);

  sendMessage(msg);

  if ((bondingStarttime + HANDSHAKETIME) > millis())
  {
    // bondingRequest was received earlier
    if (cypher == cypherPeer)
    {
      //cypher correct, so we should bond
      mesh.sendSingle(cypherNode, "Bonding"); // send bonding handshake
      bondingSuccessNode = cypherNode;
      Serial.printf("Bonding with %u\n", cypherNode);
      Serial.println("");
    }
    else
    {
      Serial.println("Our cypher does not match");
    }
  }
  else
  {
    //no bondingrequest active
    Serial.println("No open bonding request");
  }

  bondingStarttime = millis(); //bonding cypher by user so time is reset
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
  Serial.println("");

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

//maybe move the handshake into a class whith one instance per Node to track states, instead of this if/else horror

/* Controller for incoming messages that prints all incoming messages and listens on touch and bonding events */
void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  Serial.println("");

  //FIX: remove the bonding cypher that was send after the response time winoow passed
  if (msg.startsWith("Cypher"))
  { // check if another touch accordingly

    cypherPeer = msg.substring(9).toInt(); //other devices sending cyphers will override this value and block bonding -> connect to nodeid, because that is unique
    cypherNode = from;

    Serial.printf("Bonding cypher %s received from %u\n", cypherString(cypherPeer), from);

    if ((bondingStarttime + HANDSHAKETIME) > millis())
    {

      //had started a request before
      if (cypher == cypherPeer)
      {
        // bonding cypher correct, so let's bond
        mesh.sendSingle(from, "Bonding");
        Serial.printf("Bonding with %u\n", from);
        Serial.println("");
        bondingSuccessNode = from;
        if (bondingRequestNode == bondingSuccessNode)
        {
          Serial.printf("Bonded with %u\n", from);
          visualiser.cylon(cypher);
          //store in list
        }
      }
      else
      {
        Serial.println("Their cypher does not match");
      }
    }
    else
    {
      //bonding was out of time or no request from this side
      Serial.println("No open request, yet");
    }
    bondingStarttime = millis();
  }
  else if (msg.startsWith("Bonding"))
  {
    if (bondingSuccessNode == from)
    { //this has to be checked against a time
      Serial.printf("Bonded with %u\n", from);
      visualiser.cylon(cypher);
    }
    else
    {
      bondingRequestNode = from;
    }
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
