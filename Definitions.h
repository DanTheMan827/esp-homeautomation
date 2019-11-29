#ifndef DTM_DEFINITIONS
#define DTM_DEFINITIONS
#include <Arduino.h>
#include "pins_arduino.h"
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <Bounce2.h>
#include "./DNSServer.h"
#include "./WiFiStatusLED.h"
#include "./Flasher.h"
#include "./WifiStatus.h"
#include "./ConfigReset.h"
#include "./GarageDoor.h"
#include "./WirelessWatchdog.h"

#define DEBUG_ESP_PORT Serial

#define ONE_WIRE_BUS D5 // DS18B20 pin

// Garage Door
#define RELAY_PIN_1 D0
#define RELAY_PIN_2 D1
#define SENSOR_PIN_1 D2
#define SENSOR_PIN_2 D6
#define GARAGE_RELAY_PAUSE 300 // milliseconds to wait before turning the relay off

// Misc
#define ESP8266_LED D4
#define MDNS_DEBUG_RX true

// Constants
#define INFO_TYPE_GARAGEDOOR 1
#define INFO_TYPE_SWITCH 2
#define INFO_TYPE_TEMPERATURE 3
#define INFO_TYPE_THERMOSTAT 4

WiFiClient espClient;

bool RELAY_REVERSE_1 = false;
bool RELAY_REVERSE_2 = false;
bool SENSOR_REVERSE_1 = false;
bool SENSOR_REVERSE_2 = false;

#define sensorOnState(pin) (SENSOR_REVERSE_##pin ? HIGH : LOW)
#define relayOnState(pin) (RELAY_REVERSE_##pin ? HIGH : LOW)
#define offState1 (RELAY_REVERSE_1 ? LOW : HIGH)
#define onState1 (RELAY_REVERSE_1 ? HIGH : LOW)
#define offState2 (RELAY_REVERSE_2 ? LOW : HIGH)
#define onState2 (RELAY_REVERSE_2 ? HIGH : LOW)

bool lastRelayState1;
Bounce debouncer1 = Bounce();
Bounce debouncer2 = Bounce();
ESP8266WebServer server(80);
DNSServer dnsServer; // Create the DNS object
WiFiStatusLED wifiLED(ESP8266_LED, 200, 1000);
Flasher ledFlasher(ESP8266_LED, 100, 100);
WifiStatus wlStatus;
ConfigReset resetButton(10000, D3); // reset config after holding D3 LOW for 10000 milliseconds
WirelessWatchdog wirelessWatchdog(60000);
bool inform = false;
bool firstAlert = true;

// Garage Doors
GarageDoor garageDoor1(RELAY_PIN_1, SENSOR_PIN_1, GARAGE_RELAY_PAUSE);
GarageDoor garageDoor2(RELAY_PIN_2, SENSOR_PIN_2, GARAGE_RELAY_PAUSE);

String ssid;
String password;
int device_mode;
String device_description;
int infoType;
int relayCount;
bool inConfigMode;
float temperature = 0.0;
float thermostatOnTemp = 45.0;
float thermostatOffTemp = 47.0;

// MQTT Variables
PubSubClient mqttClient(espClient);
bool mqttEnabled = false;
String mqttHost;
int mqttPort;
String mqttUser;
String mqttPass;
int mqttInterval;
unsigned long mqttLastMessage;
unsigned long mqttLastConnectionAttempt;
#endif
