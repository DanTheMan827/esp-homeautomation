#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "./DNSServer.h" // Patched lib
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "WiFiStatusLED.cpp"
#include "Flasher.cpp"
#include "GarageDoor.cpp"
#include "WifiStatus.cpp"

#define DEBUG_ESP_PORT Serial

#define ONE_WIRE_BUS D5  // DS18B20 pin

// Garage Door
#define RELAY_PIN_1 D0
#define RELAY_PIN_2 D1
#define SENSOR_PIN_1 D2
#define SENSOR_PIN_2 D6
#define GARAGE_RELAY_PAUSE 300 // milliseconds to wait before turning the relay off

// Misc
#define RESET_PIN D7
#define ESP8266_LED 2
#define MDNS_DEBUG_RX true

// Constants
#define INFO_TYPE_GARAGEDOOR 1
#define INFO_TYPE_SWITCH 2
#define INFO_TYPE_TEMPERATURE 3

ESP8266WebServer server ( 80 );
DNSServer         dnsServer;              // Create the DNS object
WiFiStatusLED wifiLED(ESP8266_LED, 200, 1000);
Flasher ledFlasher(ESP8266_LED, 100, 100);
WifiStatus wlStatus;

// Garage Doors
GarageDoor garageDoor1(RELAY_PIN_1, SENSOR_PIN_1, GARAGE_RELAY_PAUSE);
GarageDoor garageDoor2(RELAY_PIN_2, SENSOR_PIN_2, GARAGE_RELAY_PAUSE);

const char* ssid;
const char* password;
int device_mode;
String device_description;
int infoType;
int relayCount;
bool inConfigMode;

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\nFree Heap Size: ";
  message += ESP.getFreeHeap();

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void pinChange(int pin) {
  Serial.print("\r\nPin ");
  Serial.print(pin);
  Serial.print(" ");
  Serial.print(digitalRead(pin));
}



void setup ( void ) {
  Serial.begin ( 115200 );


  SPIFFS.begin();

  pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(SENSOR_PIN_1, INPUT_PULLUP);
  pinMode(SENSOR_PIN_2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_1), []() {
    pinChange(SENSOR_PIN_1);
  }, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_2), []() {
    pinChange(SENSOR_PIN_2);
  }, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RESET_PIN), []() {
    if ( digitalRead(RESET_PIN) == LOW ){
      resetConfig();
    }
  }, CHANGE);

  if ( loadConfig() ) {
    inConfigMode = false;
    infoType = getInfoType();
    relayCount = getRelayCount();

    pinMode ( ONE_WIRE_BUS, INPUT_PULLUP );
    WiFi.begin ( ssid, password );
    WiFi.mode(WIFI_STA);
    
    Serial.print("\r\nChip ID: 0x");
    Serial.println(ESP.getChipId(), HEX);

    Serial.println(ssid);
    Serial.println(password);
    Serial.println(device_mode);
    Serial.println(device_description);
    
    server.on("/jquery.min.js", HTTP_GET, []() {
      handleFileRead("/jquery.min.js");
    });
    
    server.on("/info", []() {
        server.send(200, "text/json", getInfoJSON(infoType, relayCount));
      });

    if ( infoType < 5 )
      server.on("/status", [](){
        server.send(200, "text/json", getStatusJSON(infoType, relayCount));
      });

    if ( infoType == INFO_TYPE_GARAGEDOOR ){
      
      server.on("/", HTTP_GET, []() {
        handleFileRead("/garagedoor.html");
      });
      
      server.on("/trigger1", [](){ server.send ( 200, "text/json", (garageDoor1.triggerDoor() ? "true" : "false") ); });
      server.on("/open1", [](){ server.send ( 200, "text/json", (garageDoor1.openDoor() ? "true" : "false") ); });
      server.on("/close1", [](){ server.send ( 200, "text/json", (garageDoor1.closeDoor() ? "true" : "false") ); });

      if (device_mode == 2){
        server.on("/trigger2", [](){ server.send ( 200, "text/json", (garageDoor2.triggerDoor() ? "true" : "false") ); });
        server.on("/open2", [](){ server.send ( 200, "text/json", (garageDoor2.openDoor() ? "true" : "false") ); });
        server.on("/close2", [](){ server.send ( 200, "text/json", (garageDoor2.closeDoor() ? "true" : "false") ); });
      }
    }

    if ( infoType == INFO_TYPE_SWITCH ){
      server.on("/", HTTP_GET, []() {
        handleFileRead("/switch.html");
      });
    }

    if ( infoType == INFO_TYPE_TEMPERATURE ){
      server.on("/", HTTP_GET, []() {
        handleFileRead("/ds18b20.html");
      });
      server.on ( "/temperature",  handleTemperature );
    }
    server.onNotFound ( handleNotFound );
    server.begin();
    Serial.println ( "HTTP server started" );
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    wifiLED.Enable();
  } else {
    WiFi.disconnect();
    ledFlasher.Enable();
    configMode();
  }
}

int getInfoType(){
  switch(device_mode){
    case 1: return INFO_TYPE_GARAGEDOOR;
    case 2: return INFO_TYPE_GARAGEDOOR;
    case 3: return INFO_TYPE_SWITCH;
    case 4: return INFO_TYPE_SWITCH;
    case 5: return INFO_TYPE_TEMPERATURE;
  }
}

int getRelayCount(){
  switch(device_mode){
    case 1: return 1;
    case 2: return 2;
    case 3: return 1;
    case 4: return 2;
    default: return 0;
  }
}

String getStatusJSON(int infoType, int relayCount){
  DynamicJsonBuffer jsonBuffer;
  JsonArray& root = jsonBuffer.createArray();
  
  root.add(digitalRead((infoType == INFO_TYPE_GARAGEDOOR ? SENSOR_PIN_1 : RELAY_PIN_1)));
  if (relayCount > 1)
    root.add(digitalRead((infoType == INFO_TYPE_GARAGEDOOR ? SENSOR_PIN_2 : RELAY_PIN_2)));
    
  String output;
  root.printTo(output);
  return output;
}

String getInfoJSON(int infoType, int relayCount){
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["description"] = device_description;
  if ( device_mode == 1 || device_mode == 2){
    root["type"] = "garage";
  }
  if ( device_mode == 3 || device_mode == 4){
    root["type"] = "switch";
  }
  if ( device_mode == 5){
    root["type"] = "temperature";
  }
  if ( device_mode < 5)
    root["relays"] = relayCount;
  String output;
  root.printTo(output);
  return output;
}

void loop ( void ) {
  garageDoor1.Update();
  garageDoor2.Update();
  wifiLED.Update();
  wlStatus.Update();
  ledFlasher.Update();

  if (inConfigMode == true)
    dnsServer.processNextRequest();
  
  server.handleClient();
  if ( ESP.getFreeHeap() < 1000) {
    ESP.restart();
  }
}




