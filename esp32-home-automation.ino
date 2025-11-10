#include <EEPROM.h>                       // For storing relay states across reboots/flashing
#include "RMaker.h"                       // Main ESP RainMaker library (cloud IoT framework)
#include "WiFi.h"                         // Standard ESP32 WiFi
#include "WiFiProv.h"                     // ESP RainMaker BLE WiFi provisioning (mobile app connects via BLE to setup WiFi)
#include <AceButton.h>                     // Library to handle button debouncing and events
using namespace ace_button;                // For direct access to AceButton functionality

// ======== CONFIGURABLE FLAGS ==========
#define ENABLE_EEPROM true   // Whether relay states should be stored in non-volatile EEPROM (restored after reset)
#define USE_LATCHED_SWITCH false  // true: toggle switch (ON/OFF), false: push button (momentary)
#define EEPROM_SIZE 10        // Bytes reserved for EEPROM (one per relay, plus buffer)

// ======== DEVICE SETTINGS =============
const char *service_name = "ESP32_HA"; // Name broadcast by ESP32 for BLE provisioning (as seen by RainMaker app)
const char *pop = "12345678";          // Proof Of Possession (password) entered during device pairing

char deviceName_1[] = "Switch1";       // Logical/voice assistant name of relay 1 (must be unique and match Alexa/Google commands)
char deviceName_2[] = "Switch2";
char deviceName_3[] = "Switch3";
char deviceName_4[] = "Switch4";

// ========== GPIO Setup (for relays and switches, depending on hardware) ==========
static uint8_t RelayPin1 = 23;        // GPIO pin controlling relay 1 (active-low logic)
static uint8_t RelayPin2 = 19;
static uint8_t RelayPin3 = 18;
static uint8_t RelayPin4 = 5;

static uint8_t SwitchPin1 = 13;       // GPIO for physical button/switch 1
static uint8_t SwitchPin2 = 12;
static uint8_t SwitchPin3 = 14;
static uint8_t SwitchPin4 = 27;

static uint8_t wifiLed = 2;           // Optional: LED indicates WiFi connection status (HIGH=connected)
static uint8_t gpio_reset = 0;        // Factory reset pin, usually BOOT button

// ========== Relay state variables  =========
bool toggleState_1 = LOW;
bool toggleState_2 = LOW;
bool toggleState_3 = LOW;
bool toggleState_4 = LOW;

// ========== AceButton objects and configs for manual control =========
ButtonConfig config1;
AceButton button1(&config1);
ButtonConfig config2;
AceButton button2(&config2);
ButtonConfig config3;
AceButton button3(&config3);
ButtonConfig config4;
AceButton button4(&config4);

// ========== ESP RainMaker Switch Device objects (for cloud/voice control) =========
static Switch my_switch1(deviceName_1, &RelayPin1);   // Each device links logical name with a relay pin
static Switch my_switch2(deviceName_2, &RelayPin2);
static Switch my_switch3(deviceName_3, &RelayPin3);
static Switch my_switch4(deviceName_4, &RelayPin4);

//----------------------------------------------------------
// Write a relay state to EEPROM for persistence
void writeEEPROM(int addr, bool state) {
  if (ENABLE_EEPROM) {
    EEPROM.write(addr, state); // Save given state (0 or 1) at address for relay
    EEPROM.commit();           // Actually store in flash
    Serial.printf("EEPROM saved: addr %d = %d\n", addr, state);
  }
}

// Read the relay state from EEPROM
bool readEEPROM(int addr) {
  if (ENABLE_EEPROM) {
    return EEPROM.read(addr); // Get saved ON/OFF state; default is 0 if nothing written
  }
  return false;              // Fallback if EEPROM is disabled
}

// Set the relay (active LOW relays require logic inversion), and store state
void setRelay(uint8_t pin, int addr, bool state) {
  digitalWrite(pin, !state); // Relays are active-LOW: ON (true) = LOW, OFF (false) = HIGH
  if (ENABLE_EEPROM) writeEEPROM(addr, state); // Persist state
}

// Handle local button presses and update relay and RainMaker cloud
void buttonHandler(AceButton* button, uint8_t eventType, uint8_t, uint8_t relayPin, int eepromAddr, Switch &sw, bool &state) {
  bool newState = false;

  if (USE_LATCHED_SWITCH) {
    // For a latching/toggle switch, treat pressed as ON, released as OFF
    newState = (eventType == AceButton::kEventPressed);
  } else {
    // For a momentary push button, toggle state only on RELEASE
    if (eventType != AceButton::kEventReleased) return;
    newState = !(digitalRead(relayPin) == LOW); // Flip ON <-> OFF
  }

  setRelay(relayPin, eepromAddr, newState);   // Physical relay
  state = newState;                           // Track in software
  sw.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, state); // Sync RainMaker cloud/app
  Serial.printf("Relay on pin %d toggled manually to %d\n", relayPin, state);
}

void setup() {
  Serial.begin(115200);                    // Initialize serial debugging output

  if (ENABLE_EEPROM) EEPROM.begin(EEPROM_SIZE); // Initialize EEPROM buffer

  // Restore relay states on boot (for power-loss persistence)
  toggleState_1 = ENABLE_EEPROM ? readEEPROM(0) : LOW; // compact if else condition : (condition) ? (if true do this) : (if false do this);
  toggleState_2 = ENABLE_EEPROM ? readEEPROM(1) : LOW;
  toggleState_3 = ENABLE_EEPROM ? readEEPROM(2) : LOW;
  toggleState_4 = ENABLE_EEPROM ? readEEPROM(3) : LOW;

  // Configure relay and LED GPIO pins
  pinMode(RelayPin1, OUTPUT); pinMode(RelayPin2, OUTPUT);
  pinMode(RelayPin3, OUTPUT); pinMode(RelayPin4, OUTPUT);
  pinMode(wifiLed, OUTPUT);

  // Configure switch input pins with pullups
  pinMode(SwitchPin1, INPUT_PULLUP); pinMode(SwitchPin2, INPUT_PULLUP);
  pinMode(SwitchPin3, INPUT_PULLUP); pinMode(SwitchPin4, INPUT_PULLUP);
  pinMode(gpio_reset, INPUT);           // Setup BOOT/reset pin

  // Set initial relay output and sync
  setRelay(RelayPin1, 0, toggleState_1);
  setRelay(RelayPin2, 1, toggleState_2);
  setRelay(RelayPin3, 2, toggleState_3);
  setRelay(RelayPin4, 3, toggleState_4);
  digitalWrite(wifiLed, LOW);           // LED OFF (not connected)

  // Attach event handlers for each AceButton instance; link to correct relay/switch
  config1.setEventHandler([](AceButton* b, uint8_t e, uint8_t s) {
    buttonHandler(b, e, s, RelayPin1, 0, my_switch1, toggleState_1);
  });
  config2.setEventHandler([](AceButton* b, uint8_t e, uint8_t s) {
    buttonHandler(b, e, s, RelayPin2, 1, my_switch2, toggleState_2);
  });
  config3.setEventHandler([](AceButton* b, uint8_t e, uint8_t s) {
    buttonHandler(b, e, s, RelayPin3, 2, my_switch3, toggleState_3);
  });
  config4.setEventHandler([](AceButton* b, uint8_t e, uint8_t s) {
    buttonHandler(b, e, s, RelayPin4, 3, my_switch4, toggleState_4);
  });

  // Initialize AceButton drivers with correct GPIO numbers
  button1.init(SwitchPin1);
  button2.init(SwitchPin2);
  button3.init(SwitchPin3);
  button4.init(SwitchPin4);

  // --- ESP RainMaker node and device registration ---
  // All devices (switches) get attached to the node / cloud account
  Node my_node = RMaker.initNode("ESP32_Relay_4"); // Logical grouping in cloud/app
  my_switch1.addCb(write_callback);  // Register relay control callback (for Alexa/Google/app commands)
  my_switch2.addCb(write_callback);
  my_switch3.addCb(write_callback);
  my_switch4.addCb(write_callback);

  my_node.addDevice(my_switch1);     // Add device to node (so it appears in mobile app)
  my_node.addDevice(my_switch2);
  my_node.addDevice(my_switch3);
  my_node.addDevice(my_switch4);

  // --- Optional features: Over-the-air update, scheduling, timezone ---
  RMaker.enableOTA(OTA_USING_PARAMS); // Allow firmware updates via RainMaker
  RMaker.enableTZService();           // User can change timezone setting in app
  RMaker.enableSchedule();            // Enable app-based scheduling (on/off at specific times)

  RMaker.start();                     // Start RainMaker agent (connect to cloud, start core service)

  WiFi.onEvent(sysProvEvent);         // Register function to handle WiFi/BLE provisioning events
  WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE,
                         NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM,
                         NETWORK_PROV_SECURITY_1, pop, service_name); // Start BLE provisioning for onboarding

  // Report current state to RainMaker so app/cloud is in sync
  my_switch1.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_1);
  my_switch2.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_2);
  my_switch3.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_3);
  my_switch4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, toggleState_4);

  Serial.println("Setup completed with EEPROM and mode flags.");
}

void loop() {
  // --- BOOT button long-press triggers soft or full reset ---
  if (digitalRead(gpio_reset) == LOW) {
    delay(100);
    int startTime = millis();
    while (digitalRead(gpio_reset) == LOW) delay(50);
    int duration = millis() - startTime;
    if (duration > 10000) {
      Serial.println("Factory reset triggered.");
      RMakerFactoryReset(2); // Erase all config, require reprovisioning
    } else if (duration > 3000) {
      Serial.println("WiFi reset triggered.");
      RMakerWiFiReset(2);    // Only WiFi credentials cleared, keep RainMaker config
    }
  }

  digitalWrite(wifiLed, WiFi.status() == WL_CONNECTED); // Update WiFi LED

  // Check/playback all button event state machines
  button1.check();
  button2.check();
  button3.check();
  button4.check();
}

// ========== Callback: handle RainMaker/app/alexa/google command to switch ==========
void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx) {
  const char *device_name = device->getDeviceName();   // Identify which logical switch called the callback
  const char *param_name = param->getParamName();      // Identify which parameter is being changed ("Power")

  if(strcmp(param_name, "Power") == 0) {               // For ON/OFF commands only
    bool newState = val.val.b;                         // Extract true/false state
    // Find which relay to update and set relay + sync with RainMaker cloud/app
    if (strcmp(device_name, deviceName_1) == 0) {
      setRelay(RelayPin1, 0, newState); toggleState_1 = newState; my_switch1.updateAndReportParam(param_name, newState);
    } else if (strcmp(device_name, deviceName_2) == 0) {
      setRelay(RelayPin2, 1, newState); toggleState_2 = newState; my_switch2.updateAndReportParam(param_name, newState);
    } else if (strcmp(device_name, deviceName_3) == 0) {
      setRelay(RelayPin3, 2, newState); toggleState_3 = newState; my_switch3.updateAndReportParam(param_name, newState);
    } else if (strcmp(device_name, deviceName_4) == 0) {
      setRelay(RelayPin4, 3, newState); toggleState_4 = newState; my_switch4.updateAndReportParam(param_name, newState);
    }
    Serial.printf("Write callback for %s: new state = %d\n", device_name, newState); // Debug output
  }
}

// ========== WiFi/RainMaker BLE provisioning event handler ==========
void sysProvEvent(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      Serial.printf("Provisioning started: %s\n", service_name);
      printQR(service_name, pop, "ble");    // Output QR code in serial for app pairing
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("Connected to Wi-Fi");
      digitalWrite(wifiLed, true);
      break;
  }
}
