#define DEBUG_ESP_PORT Serial

#define ONE_WIRE_BUS D5  // DS18B20 pin

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

bool RELAY_1_REVERSE = false;
bool RELAY_2_REVERSE = false;
bool SENSOR_1_REVERSE = false;
bool SENSOR_2_REVERSE = false;

int offState1 = (RELAY_1_REVERSE ? LOW : HIGH);
int onState1 = (RELAY_1_REVERSE ? HIGH : LOW);
int offState2 = (RELAY_2_REVERSE ? LOW : HIGH);
int onState2 = (RELAY_2_REVERSE ? HIGH : LOW);

bool lastRelayState1;

ESP8266WebServer server ( 80 );
DNSServer dnsServer; // Create the DNS object
WiFiStatusLED wifiLED(ESP8266_LED, 200, 1000);
Flasher ledFlasher(ESP8266_LED, 100, 100);
WifiStatus wlStatus;
ConfigReset resetButton(10000, D3); // reset config after holding D3 LOW for 10000 milliseconds
NoWirelessReboot wirelessWatchdog(60000);
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

void handleNotFound() {
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\nFree Heap Size: ");
  message += ESP.getFreeHeap();

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, FF("text/plain"), message );
}

bool isPinOn(int pin, bool reverse) {
  int offState = (reverse ? LOW : HIGH);
  int onState = (reverse ? HIGH : LOW);

  int state = digitalRead(pin);

  return (state == onState);
}

void setup ( void ) {
  Serial.begin ( 74880 );

  SPIFFS.begin();

  pinMode(SENSOR_PIN_1, INPUT_PULLUP);
  pinMode(SENSOR_PIN_2, INPUT_PULLUP);
  lastRelayState1 = digitalRead(RELAY_PIN_1);

  if (loadConfig()) {
    wirelessWatchdog.Enable();
    inConfigMode = false;
    infoType = getInfoType();
    relayCount = getRelayCount();

    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_1), []() {
      if (infoType == INFO_TYPE_SWITCH) {
        if (isPinOn(SENSOR_PIN_1, SENSOR_1_REVERSE)) {
          if (isPinOn(RELAY_PIN_1, RELAY_1_REVERSE)) {
            digitalWrite(RELAY_PIN_1, offState1);
          } else {
            digitalWrite(RELAY_PIN_1, onState1);
          }
        }
      }
    }, CHANGE);
    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_2), []() {
      if (infoType == INFO_TYPE_SWITCH && relayCount > 1) {
        if (isPinOn(SENSOR_PIN_2, SENSOR_2_REVERSE)) {
          if (isPinOn(RELAY_PIN_2, RELAY_2_REVERSE)) {
            digitalWrite(RELAY_PIN_2, offState2);
          } else {
            digitalWrite(RELAY_PIN_2, onState2);
          }
        }
      }
    }, CHANGE);

    pinMode ( ONE_WIRE_BUS, INPUT_PULLUP );
    WiFi.begin ( ssid.c_str(), password.c_str() );
    WiFi.mode(WIFI_STA);

    Serial.print(F("\r\nChip ID: 0x"));
    Serial.println(ESP.getChipId(), HEX);

    Serial.println(ssid);
    Serial.println(password);
    Serial.println(device_mode);
    Serial.println(device_description);

    if (mqttHost != "") {
      mqttEnabled = true;
      mqttClient.setServer(mqttHost.c_str(), mqttPort);
      mqttClient.setCallback(mqttCallback);
    }

    ServerOnGet("/jquery.min.js", {
      handleFileRead("/jquery.min.js");
    });

    ServerOnGet("/info", {
      ServerSendJson(getInfoJSON(infoType, relayCount));
    });

    if ( infoType < 5 || infoType == INFO_TYPE_THERMOSTAT ) {
      ServerOnGet("/status", {
        ServerSendJson(getStatusJSON(infoType, relayCount));
      });
    }

    if ( infoType == INFO_TYPE_GARAGEDOOR ) {

      ServerOnGet("/", {
        handleFileRead("/garagedoor.html");
      });

      ServerOnGet("/trigger1", {
        ServerSendJson(garageDoor1.triggerDoor() ? TRUE_STRING : FALSE_STRING);
      });
      ServerOnGet("/open1", {
        ServerSendJson(garageDoor1.openDoor() ? TRUE_STRING : FALSE_STRING);
      });
      ServerOnGet("/close1", {
        ServerSendJson(garageDoor1.closeDoor() ? TRUE_STRING : FALSE_STRING);
      });

      if (device_mode == 2) {
        ServerOnGet("/trigger2", {
          ServerSendJson(garageDoor2.triggerDoor() ? TRUE_STRING : FALSE_STRING);
        });
        ServerOnGet("/open2", {
          ServerSendJson(garageDoor2.openDoor() ? TRUE_STRING : FALSE_STRING);
        });
        ServerOnGet("/close2", {
          ServerSendJson(garageDoor2.closeDoor() ? TRUE_STRING : FALSE_STRING);
        });
      }
    }

    if (infoType == INFO_TYPE_SWITCH) {
      pinMode(RELAY_PIN_1, OUTPUT);
      digitalWrite(RELAY_PIN_1, offState1);
      mqttPublish("Relay1", "0");

      ServerOnGet("/status1", {
        ServerSendJson(digitalRead(RELAY_PIN_1) == onState1 ? "1" : "0");
      });

      ServerOnGet("/on1", {
        digitalWrite(RELAY_PIN_1, onState1);
        ServerSendJson(TRUE_STRING);
        mqttPublish("Relay1", "1");
      });

      ServerOnGet("/off1", {
        digitalWrite(RELAY_PIN_1, offState1);
        ServerSendJson(TRUE_STRING);
        mqttPublish("Relay1", "0");
      });

      ServerOnGet("/toggle1", {
        digitalWrite(RELAY_PIN_1, !digitalRead(RELAY_PIN_1));
        ServerSendJson(TRUE_STRING);
        mqttPublish("Relay1", isPinOn(RELAY_PIN_1, RELAY_1_REVERSE) ? "1" : "0");
      });

      if (relayCount > 1) {
        pinMode(RELAY_PIN_2, OUTPUT);
        digitalWrite(RELAY_PIN_2, offState2);

        //attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_2), []() {
        //if(isPinOn(RELAY_PIN_2, RELAY_2_REVERSE)){
        //digitalWrite(RELAY_PIN_2, onState2);
        //} else {
        //digitalWrite(RELAY_PIN_2, offState2);
        //}
        //}, CHANGE);

        ServerOnGet("/status2", {
          ServerSendJson(digitalRead(RELAY_PIN_2) == onState2 ? "1" : "0");
        });

        ServerOnGet("/on2", {
          digitalWrite(RELAY_PIN_2, onState2);
          ServerSendJson(TRUE_STRING);
          mqttPublish("Relay2", "1");
        });

        ServerOnGet("/off2", {
          digitalWrite(RELAY_PIN_2, offState2);
          ServerSendJson(TRUE_STRING);
          mqttPublish("Relay2", "0");
        });

        ServerOnGet("/toggle2", {
          digitalWrite(RELAY_PIN_2, !digitalRead(RELAY_PIN_2));
          ServerSendJson(TRUE_STRING);
          mqttPublish("Relay2", isPinOn(RELAY_PIN_2, RELAY_2_REVERSE) ? "1" : "0");
        });
      }

      ServerOnGet("/", {
        handleFileRead("/switch.html");
      });

    }

    if ( infoType == INFO_TYPE_TEMPERATURE || infoType == INFO_TYPE_THERMOSTAT ) {
      ServerOnGet("/", {
        handleFileRead("/ds18b20.html");
      });

      server.on(FF("/temperature"), handleTemperature);

      if ( infoType == INFO_TYPE_THERMOSTAT ) {
        ServerOnGet("/setthermostat", {
          ServerSendJson(setThermostatTemps(server.hasArg("on") ? server.arg("on").toFloat() : -100.0, server.hasArg("off") ? server.arg("off").toFloat() : -100.0) ? TRUE_STRING : FALSE_STRING);
        });
      }
    }
    server.onNotFound ( handleNotFound );
    server.begin();
    Serial.println ( "HTTP server started" );
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    wifiLED.Enable();
    mqttLastMessage = millis() - (mqttInterval * 1000);
    mqttLastConnectionAttempt = millis() - 10000;

  } else {
    WiFi.disconnect();
    ledFlasher.Enable();
    configMode();
  }
}

int getInfoType() {
  switch (device_mode) {
    case 1: return INFO_TYPE_GARAGEDOOR;
    case 2: return INFO_TYPE_GARAGEDOOR;
    case 3: return INFO_TYPE_SWITCH;
    case 4: return INFO_TYPE_SWITCH;
    case 5: return INFO_TYPE_TEMPERATURE;
    case 6: return INFO_TYPE_THERMOSTAT;
  }
}

int getRelayCount() {
  switch (device_mode) {
    case 1: return 1;
    case 2: return 2;
    case 3: return 1;
    case 4: return 2;
    default: return 0;
  }
}

String getStatusJSON(int infoType, int relayCount) {
  DynamicJsonBuffer jsonBuffer;
  JsonArray& root = jsonBuffer.createArray();

  if (infoType == INFO_TYPE_GARAGEDOOR) {
    root.add(garageDoor1.isOpen() ? 1 : 0);
  } else {
    root.add(isPinOn(RELAY_PIN_1, RELAY_1_REVERSE) ? 0 : 1);
  }

  if (relayCount > 1) {
    if (infoType == INFO_TYPE_GARAGEDOOR) {
      root.add(garageDoor2.isOpen() ? 1 : 0);
    } else {
      root.add(isPinOn(RELAY_PIN_2, RELAY_2_REVERSE) ? 0 : 1);
    }
  }

  String output;
  root.printTo(output);
  return output;
}

String getInfoJSON(int infoType, int relayCount) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["description"] = device_description;
  if ( device_mode == 1 || device_mode == 2) {
    root["type"] = "garage";
  }
  if ( device_mode == 3 || device_mode == 4) {
    root["type"] = "switch";
  }
  if ( device_mode == 5) {
    root["type"] = "temperature";
  }
  if ( device_mode == 6) {
    root["type"] = "thermostat";
    root["onTemp"] = thermostatOnTemp;
    root["offTemp"] = thermostatOffTemp;
  }
  if ( device_mode < 5)
    root["relays"] = relayCount;
  String output;
  root.printTo(output);
  return output;
}

void mqttReconnect() {
  // Loop until we're reconnected
  Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "esp-";
  clientId += String(ESP.getChipId(), HEX);
  // Attempt to connect
  Serial.print(clientId);
  Serial.print(mqttUser);
  if (mqttClient.connect(clientId.c_str(), mqttUser.c_str(), mqttPass.c_str(), mqttGetTopic("Status").c_str(), 1, true, "Offline")) {
    Serial.println("connected");
    mqttClient.subscribe(mqttGetTopic("Command").c_str());
    mqttPublish("Description", device_description.c_str());
    if ( device_mode == 1 || device_mode == 2) {
      mqttPublish("Type", "garage");
    }
    if ( device_mode == 3 || device_mode == 4) {
      mqttPublish("Type", "switch");
    }
    if ( device_mode == 5) {
      mqttPublish("Type", "temperature");
    }
    if ( device_mode == 6) {
      mqttPublish("Type", "thermostat");
      {
        char tempString[10];
        sprintf(tempString, "%f", thermostatOnTemp);
        mqttPublish("OnTemp", tempString);
      }
      {
        char tempString[10];
        sprintf(tempString, "%f", thermostatOffTemp);
        mqttPublish("OffTemp", tempString);
      }
    }
    if ( device_mode < 5) {
      char tempString[10];
      sprintf(tempString, "%d", relayCount);
      mqttPublish("Relays", tempString);
    }
  } else {
    Serial.print("failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" try again in 10 seconds");
  }
}

String mqttGetTopic(const char* topic) {
  String outTopic;
  outTopic = "esp-";
  outTopic += String(ESP.getChipId(), HEX);
  outTopic += "/";
  outTopic += topic;
  return outTopic;
}

bool setThermostatTemps(float onTemp, float offTemp) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  if (onTemp != -100.0)
    thermostatOnTemp = onTemp;

  if (offTemp != -100.0)
    thermostatOffTemp = offTemp;
  
  json["wap"] = ssid;
  json["wpsk"] = password;
  json["mode"] = device_mode;
  json["desc"] = device_description;
  json["mqttHost"] = mqttHost;
  json["mqttPort"] = mqttPort;
  json["mqttUser"] = mqttUser;
  json["mqttPass"] = mqttPass;
  json["mqttInt"] = mqttInterval;
  json["invSense1"] = SENSOR_1_REVERSE;
  json["invSense2"] = SENSOR_2_REVERSE;
  json["invRelay1"] = RELAY_1_REVERSE;
  json["invRelay2"] = RELAY_2_REVERSE;
  json["onTemp"] = thermostatOnTemp;
  json["offTemp"] = thermostatOffTemp;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFile);
  configFile.close();

  return true;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]\n");

  if ( infoType == INFO_TYPE_GARAGEDOOR ) {
    mqttIf("status", {
      mqttPublish("Sensor1", garageDoor1.isOpen() ? "1" : "0");
      if (device_mode == 2) {
        mqttPublish("Sensor2", garageDoor2.isOpen() ? "1" : "0");
      }
    });

    mqttIf("trigger1", {
      garageDoor1.triggerDoor();
      return;
    });
    mqttIf("open1", {
      garageDoor1.openDoor();
      return;
    });
    mqttIf("close1", {
      garageDoor1.closeDoor();
      return;
    });
    if (device_mode == 2) {
      mqttIf("trigger2", {
        garageDoor2.triggerDoor();
        return;
      });
      mqttIf("open2", {
        garageDoor2.openDoor();
        return;
      });
      mqttIf("close2", {
        garageDoor2.closeDoor();
        return;
      });
    }
  }

  if ( infoType == INFO_TYPE_SWITCH ) {
    mqttIf("status", {
      mqttPublish("Relay1", digitalRead(RELAY_PIN_1) == onState1 ? "1" : "0");
      if (relayCount > 1) {
        mqttPublish("Relay2", digitalRead(RELAY_PIN_2) == onState2 ? "1" : "0");
      }
      return;
    });
    mqttIf("status1", {
      mqttPublish("Relay1", digitalRead(RELAY_PIN_1) == onState1 ? "1" : "0");
      return;
    });
    mqttIf("on1", {
      digitalWrite(RELAY_PIN_1, onState1);
      mqttPublish("Relay1", "1");
      return;
    });
    mqttIf("off1", {
      digitalWrite(RELAY_PIN_1, offState1);
      mqttPublish("Relay1", "0");
      return;
    });
    mqttIf("toggle1", {
      digitalWrite(RELAY_PIN_1, !digitalRead(RELAY_PIN_1));
      mqttPublish("Relay1", isPinOn(RELAY_PIN_1, RELAY_1_REVERSE) ? "1" : "0");
      return;
    });

    if (relayCount > 1) {
      mqttIf("status2", {
        mqttPublish("Relay2", digitalRead(RELAY_PIN_2) == onState2 ? "1" : "0");
        return;
      });
      mqttIf("on2", {
        digitalWrite(RELAY_PIN_2, onState2);
        mqttPublish("Relay2", "1");
        return;
      });
      mqttIf("off2", {
        digitalWrite(RELAY_PIN_2, offState2);
        mqttPublish("Relay2", "0");
        return;
      });
      mqttIf("toggle2", {
        digitalWrite(RELAY_PIN_2, !digitalRead(RELAY_PIN_2));
        mqttPublish("Relay2", isPinOn(RELAY_PIN_2, RELAY_2_REVERSE) ? "1" : "0");
        return;
      });
    }
  }

  if (infoType == INFO_TYPE_THERMOSTAT) {
    if (length > 7 && !strncmp("onTemp=", (const char*)payload, 7)) {
      std::string payloadString((const char*)(payload + 7), length - 7);
      Serial.println(payloadString.c_str());
      bool rt = setThermostatTemps(atof(payloadString.c_str()), -100.0);

      char tempString[10];
      sprintf(tempString, "%f", thermostatOnTemp);
      mqttPublish("OnTemp", tempString);
    }
    if (length > 8 && !strncmp("offTemp=", (const char*)payload, 8)) {
      std::string payloadString((const char*)(payload + 8), length - 8);
      Serial.println(payloadString.c_str());
      bool rt = setThermostatTemps(-100.0, atof(payloadString.c_str()));

      char tempString[10];
      sprintf(tempString, "%f", thermostatOffTemp);
      mqttPublish("OffTemp", tempString);
    }
  }

  if (infoType == INFO_TYPE_TEMPERATURE || infoType == INFO_TYPE_THERMOSTAT) {
    mqttIf("status", {
      mqttPublish("Temperature", getTemperatureString().c_str());
      return;
    });
  }

  if ( infoType < 5 || infoType == INFO_TYPE_THERMOSTAT ) {
    mqttIf("status", {
      mqttPublish("status", getStatusJSON(infoType, relayCount).c_str());
      return;
    });
  }

  mqttIf("reboot", {
    mqttPublish("Status", "Offline");
    ESP.reset();
  });
}

void mqttPublish(const char* topic, const char* message) {
  if (!(mqttEnabled && inConfigMode == false && WiFi.status() == WL_CONNECTED)) {
    return;
  }

  if (mqttClient.connected()) {
    printf("Topic: %s\nMessage: %s\n\n", topic, message);
    mqttClient.publish(mqttGetTopic(topic).c_str(), message, true);
  }
}

void handleMqttPub() {
  mqttPublish("Status", "Online");
  if (infoType == INFO_TYPE_GARAGEDOOR) {
    mqttPublish("Sensor1", garageDoor1.isOpen() ? "1" : "0");
  } else if (infoType != INFO_TYPE_TEMPERATURE) {
    mqttPublish("Relay1", isPinOn(RELAY_PIN_1, RELAY_1_REVERSE) ? "1" : "0");
  }

  if (relayCount > 1) {
    if (infoType == INFO_TYPE_GARAGEDOOR) {
      mqttPublish("Sensor2", garageDoor2.isOpen() ? "1" : "0");
    } else if (infoType != INFO_TYPE_TEMPERATURE) {
      mqttPublish("Relay2", isPinOn(RELAY_PIN_2, RELAY_2_REVERSE) ? "1" : "0");
    }
  }

  if (infoType == INFO_TYPE_TEMPERATURE || infoType == INFO_TYPE_THERMOSTAT) {
    mqttPublish("Temperature", getTemperatureString().c_str());
    if (infoType == INFO_TYPE_THERMOSTAT) {
      mqttPublish("Relay1", isPinOn(RELAY_PIN_2, RELAY_2_REVERSE) ? "0" : "1");
      if (relayCount > 1) {
        mqttPublish("Relay2", isPinOn(RELAY_PIN_2, RELAY_2_REVERSE) ? "0" : "1");
      }
      {
        char tempString[10];
        sprintf(tempString, "%f", thermostatOnTemp);
        mqttPublish("OnTemp", tempString);
      }
      {
        char tempString[10];
        sprintf(tempString, "%f", thermostatOffTemp);
        mqttPublish("OffTemp", tempString);
      }
    }
  }
}

void loop ( void ) {

  if ( infoType == INFO_TYPE_GARAGEDOOR ) {
    if (garageDoor1.Update()) {
      mqttPublish("Sensor1", garageDoor1.isOpen() ? "1" : "0");
    }
    if (garageDoor2.Update()) {
      mqttPublish("Sensor2", garageDoor2.isOpen() ? "1" : "0");
    }

  }
  wifiLED.Update();
  wlStatus.Update();
  ledFlasher.Update();
  resetButton.Update();
  wirelessWatchdog.Update();
  if (inConfigMode == true) {
    dnsServer.processNextRequest();
  } else if ( infoType == INFO_TYPE_THERMOSTAT ) {
    temperature = getTemperature();

    if (temperature < thermostatOnTemp) {
      digitalWrite(RELAY_PIN_1, (RELAY_1_REVERSE ? LOW : HIGH));
    } else if (temperature > thermostatOffTemp) {
      digitalWrite(RELAY_PIN_1, (RELAY_1_REVERSE ? HIGH : LOW));
    }
    bool currentRelay1State = digitalRead(RELAY_PIN_1);
    if (lastRelayState1 != currentRelay1State) {
      lastRelayState1 = currentRelay1State;
      mqttPublish("Relay1", isPinOn(RELAY_PIN_1, RELAY_1_REVERSE) ? "1" : "0");
    }
  }

  unsigned long currentMillis = millis();
  if (mqttEnabled && inConfigMode == false && WiFi.status() == WL_CONNECTED) {
    if (mqttClient.connected()) {
      if (currentMillis - mqttLastMessage >= (mqttInterval * 1000)) {
        mqttLastMessage = currentMillis;   // Remember the time
        char message[25];
        handleMqttPub();
      }
    } else {
      if (currentMillis - mqttLastConnectionAttempt >= 10000) {
        mqttLastConnectionAttempt = currentMillis;
        mqttReconnect();
      }
    }
    mqttClient.loop();
  }

  server.handleClient();

  if ( ESP.getFreeHeap() < 1000) {
    ESP.restart();
  }
}
