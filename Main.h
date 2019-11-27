#ifndef DTM_MAIN
#define DTM_MAIN
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <pins_arduino.h>
#include "./Definitions.h"
#include "./Macros.h"
#include "./floatToString.h"
#include "./Temperature.h"
#include "./Config.h"
#include "./ServeFilesFromFlash.h"

bool setThermostatTemps(float onTemp, float offTemp)
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();

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
  json["invSense1"] = SENSOR_REVERSE_1;
  json["invSense2"] = SENSOR_REVERSE_2;
  json["invRelay1"] = RELAY_REVERSE_1;
  json["invRelay2"] = RELAY_REVERSE_2;
  json["onTemp"] = thermostatOnTemp;
  json["offTemp"] = thermostatOffTemp;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFile);
  configFile.close();

  return true;
}

String getStatusJSON(int infoType, int relayCount)
{
  DynamicJsonBuffer jsonBuffer;
  JsonArray &root = jsonBuffer.createArray();

  if (infoType == INFO_TYPE_GARAGEDOOR)
  {
    root.add(garageDoor1.isOpen() ? 1 : 0);
  }
  else
  {
    root.add(isRelayOn(1) ? 1 : 0);
  }

  if (relayCount > 1)
  {
    if (infoType == INFO_TYPE_GARAGEDOOR)
    {
      root.add(garageDoor2.isOpen() ? 1 : 0);
    }
    else
    {
      root.add(isRelayOn(2) ? 1 : 0);
    }
  }

  String output;
  root.printTo(output);
  return output;
}

String getInfoJSON(int infoType, int relayCount)
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["description"] = device_description;
  if (device_mode == 1 || device_mode == 2)
  {
    root["type"] = "garage";
  }

  if (device_mode == 3 || device_mode == 4)
  {
    root["type"] = "switch";
  }

  if (device_mode == 5)
  {
    root["type"] = "temperature";
  }

  if (device_mode == 6)
  {
    root["type"] = "thermostat";
    root["onTemp"] = thermostatOnTemp;
    root["offTemp"] = thermostatOffTemp;
  }

  if (device_mode < 5)
  {
    root["relays"] = relayCount;
  }
  String output;
  root.printTo(output);
  return output;
}

int getRelayCount()
{
  switch (device_mode)
  {
  case 1:
    return 1;

  case 2:
    return 2;

  case 3:
    return 1;

  case 4:
    return 2;

  default:
    return 0;
  }
}

int getInfoType()
{
  switch (device_mode)
  {
  case 1:
    return INFO_TYPE_GARAGEDOOR;

  case 2:
    return INFO_TYPE_GARAGEDOOR;

  case 3:
    return INFO_TYPE_SWITCH;

  case 4:
    return INFO_TYPE_SWITCH;

  case 5:
    return INFO_TYPE_TEMPERATURE;

  case 6:
    return INFO_TYPE_THERMOSTAT;
  }
}

String mqttGetTopic(const char *topic)
{
  String outTopic;
  outTopic = "esp-";
  outTopic += String(ESP.getChipId(), HEX);
  outTopic += "/";
  outTopic += topic;
  return outTopic;
}

void mqttPublish(const char *topic, const char *message)
{
  if (!(mqttEnabled && inConfigMode == false && WiFi.status() == WL_CONNECTED))
  {
    return;
  }

  if (mqttClient.connected())
  {
    printf("Topic: %s\nMessage: %s\n\n", topic, message);
    mqttClient.publish(mqttGetTopic(topic).c_str(), message, true);
  }
}

void handleMqttPub()
{
  mqttPublish("LWT", "Online");
  if (infoType == INFO_TYPE_GARAGEDOOR)
  {
    mqttPublish("Sensor1", garageDoor1.isOpen() ? "1" : "0");
  }
  else if (infoType != INFO_TYPE_TEMPERATURE)
  {
    mqttPublish("Relay1", isRelayOn(1) ? "1" : "0");
  }

  if (relayCount > 1)
  {
    if (infoType == INFO_TYPE_GARAGEDOOR)
    {
      mqttPublish("Sensor2", garageDoor2.isOpen() ? "1" : "0");
    }
    else if (infoType != INFO_TYPE_TEMPERATURE)
    {
      mqttPublish("Relay2", isRelayOn(2) ? "1" : "0");
    }
  }

  if (infoType == INFO_TYPE_TEMPERATURE || infoType == INFO_TYPE_THERMOSTAT)
  {
    mqttPublishFloat("Temperature", getTemperature());
    if (infoType == INFO_TYPE_THERMOSTAT)
    {
      mqttPublish("Relay1", isRelayOn(1) ? "1" : "0");
      if (relayCount > 1)
      {
        mqttPublish("Relay2", isRelayOn(2) ? "1" : "0");
      }
      mqttPublishFloat("OnTemp", thermostatOnTemp);
      mqttPublishFloat("OffTemp", thermostatOffTemp);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]\n");

  if (infoType == INFO_TYPE_GARAGEDOOR)
  {
    mqttIf("status", {
      mqttPublish("Sensor1", garageDoor1.isOpen() ? "1" : "0");
      if (device_mode == 2)
      {
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
    if (device_mode == 2)
    {
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

  if (infoType == INFO_TYPE_SWITCH)
  {
    mqttIf("status", {
      mqttPublish("Relay1", isRelayOn(1) ? "1" : "0");
      if (relayCount > 1)
      {
        mqttPublish("Relay2", isRelayOn(2) ? "1" : "0");
      }
      return;
    });

    mqttIf("status1", {
      mqttPublish("Relay1", isRelayOn(1) ? "1" : "0");
      return;
    });

    mqttIf("on1", {
      relayOn(1);
      mqttPublish("Relay1", "1");
      return;
    });

    mqttIf("off1", {
      relayOff(1);
      mqttPublish("Relay1", "0");
      return;
    });

    mqttIf("toggle1", {
      relayToggle(1);
      mqttPublish("Relay1", isRelayOn(1) ? "1" : "0");
      return;
    });

    if (relayCount > 1)
    {
      mqttIf("status2", {
        mqttPublish("Relay2", isRelayOn(2) ? "1" : "0");
        return;
      });

      mqttIf("on2", {
        relayOn(2);
        mqttPublish("Relay2", "1");
        return;
      });

      mqttIf("off2", {
        relayOff(2);
        mqttPublish("Relay2", "0");
        return;
      });

      mqttIf("toggle2", {
        relayToggle(2);
        mqttPublish("Relay2", isRelayOn(2) ? "1" : "0");
        return;
      });
    }
  }

  if (infoType == INFO_TYPE_THERMOSTAT)
  {
    if (length > 7 && !strncmp("onTemp=", (const char *)payload, 7))
    {
      std::string payloadString((const char *)(payload + 7), length - 7);
      Serial.println(payloadString.c_str());
      bool rt = setThermostatTemps(atof(payloadString.c_str()), -100.0);
      mqttPublishFloat("OnTemp", thermostatOnTemp);
    }
    if (length > 8 && !strncmp("offTemp=", (const char *)payload, 8))
    {
      std::string payloadString((const char *)(payload + 8), length - 8);
      Serial.println(payloadString.c_str());
      bool rt = setThermostatTemps(-100.0, atof(payloadString.c_str()));

      mqttPublishFloat("OffTemp", thermostatOffTemp);
    }
  }

  if (infoType == INFO_TYPE_TEMPERATURE)
  {
    mqttIf("status", {
      mqttPublishFloat("Temperature", getTemperature());
      return;
    });
  }

  if (infoType == INFO_TYPE_THERMOSTAT)
  {
    mqttIf("status", {
      mqttPublishFloat("Temperature", getTemperature());
      mqttPublish("Relay1", isRelayOn(1) ? "1" : "0");
      if (relayCount > 1)
      {
        mqttPublish("Relay2", isRelayOn(2) ? "1" : "0");
      }
      mqttPublishFloat("OnTemp", thermostatOnTemp);
      mqttPublishFloat("OffTemp", thermostatOffTemp);
      return;
    });
  }

  if (infoType < 5 || infoType == INFO_TYPE_THERMOSTAT)
  {
    mqttIf("status", {
      mqttPublish("status", getStatusJSON(infoType, relayCount).c_str());
      return;
    });
  }

  mqttIf("reboot", {
    mqttPublish("LWT", "Offline");
    ESP.restart();
  });
}

void setup(void)
{
  Serial.begin(74880);

  SPIFFS.begin();

  pinMode(SENSOR_PIN_1, INPUT_PULLUP);
  pinMode(SENSOR_PIN_2, INPUT_PULLUP);
  lastRelayState1 = digitalRead(RELAY_PIN_1);

  if (loadConfig())
  {
    wirelessWatchdog.Enable();
    inConfigMode = false;
    infoType = getInfoType();
    relayCount = getRelayCount();

    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_1), []() {
      if (infoType == INFO_TYPE_SWITCH)
      {
        if (isSensorOn(1))
        {
          if (isRelayOn(1))
          {
            relayOff(1);
          }
          else
          {
            relayOn(1);
          }
        }
        mqttPublish("Relay1", isRelayOn(1) ? "1" : "0");
      }
    },
                    CHANGE);
    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN_2), []() {
      if (infoType == INFO_TYPE_SWITCH && relayCount > 1)
      {
        if (isSensorOn(2))
        {
          if (isRelayOn(2))
          {
            relayOff(2);
          }
          else
          {
            relayOn(2);
          }
        }
        mqttPublish("Relay2", isRelayOn(2) ? "1" : "0");
      }
    },
                    CHANGE);

    pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
    WiFi.begin(ssid.c_str(), password.c_str());
    WiFi.mode(WIFI_STA);

    Serial.print(F("\r\nChip ID: 0x"));
    Serial.println(ESP.getChipId(), HEX);

    Serial.println(ssid);
    Serial.println(password);
    Serial.println(device_mode);
    Serial.println(device_description);

    if (mqttHost != "")
    {
      mqttEnabled = true;
      mqttClient.setServer(mqttHost.c_str(), mqttPort);
      mqttClient.setCallback(mqttCallback);
    }

    ServerSendSpiff("/jquery.min.js", "/jquery.min.js");

    ServerOnGet("/info", {
      ServerSendJson(getInfoJSON(infoType, relayCount));
    });

    if (infoType < 5 || infoType == INFO_TYPE_THERMOSTAT)
    {
      ServerOnGet("/status", {
        ServerSendJson(getStatusJSON(infoType, relayCount));
      });
    }

    if (infoType == INFO_TYPE_GARAGEDOOR)
    {

      ServerSendSpiff("/", "/garagedoor.html");

      ServerOnGet("/trigger1", {
        ServerSendJson(garageDoor1.triggerDoor() ? TRUE_STRING : FALSE_STRING);
      });
      ServerOnGet("/open1", {
        ServerSendJson(garageDoor1.openDoor() ? TRUE_STRING : FALSE_STRING);
      });
      ServerOnGet("/close1", {
        ServerSendJson(garageDoor1.closeDoor() ? TRUE_STRING : FALSE_STRING);
      });

      if (device_mode == 2)
      {
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

    if (infoType == INFO_TYPE_SWITCH)
    {
      pinMode(RELAY_PIN_1, OUTPUT);
      relayOff(1);
      mqttPublish("Relay1", "0");

      ServerOnGet("/status1", {
        ServerSendJson(isRelayOn(1) ? "1" : "0");
      });

      ServerOnGet("/on1", {
        relayOn(1);
        ServerSendJson(TRUE_STRING);
        mqttPublish("Relay1", "1");
      });

      ServerOnGet("/off1", {
        relayOff(1);
        ServerSendJson(TRUE_STRING);
        mqttPublish("Relay1", "0");
      });

      ServerOnGet("/toggle1", {
        relayToggle(1);
        ServerSendJson(TRUE_STRING);
        mqttPublish("Relay1", isRelayOn(1) ? "1" : "0");
      });

      if (relayCount > 1)
      {
        pinMode(RELAY_PIN_2, OUTPUT);
        relayOff(2);

        ServerOnGet("/status2", {
          ServerSendJson(isRelayOn(2) ? "1" : "0");
        });

        ServerOnGet("/on2", {
          relayOn(2);
          ServerSendJson(TRUE_STRING);
          mqttPublish("Relay2", "1");
        });

        ServerOnGet("/off2", {
          relayOff(2);
          ServerSendJson(TRUE_STRING);
          mqttPublish("Relay2", "0");
        });

        ServerOnGet("/toggle2", {
          relayToggle(2);
          ServerSendJson(TRUE_STRING);
          mqttPublish("Relay2", isRelayOn(2) ? "1" : "0");
        });
      }

      ServerSendSpiff("/", "/switch.html");
    }

    if (infoType == INFO_TYPE_TEMPERATURE || infoType == INFO_TYPE_THERMOSTAT)
    {
      ServerSendSpiff("/", "/ds18b20.html");

      server.on(FF("/temperature"), handleTemperature);

      if (infoType == INFO_TYPE_THERMOSTAT)
      {
        ServerOnGet("/setthermostat", {
          ServerSendJson(setThermostatTemps(server.hasArg("on") ? server.arg("on").toFloat() : -100.0, server.hasArg("off") ? server.arg("off").toFloat() : -100.0) ? TRUE_STRING : FALSE_STRING);
        });
      }
    }
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    wifiLED.Enable();
    mqttLastMessage = millis() - (mqttInterval * 1000);
    mqttLastConnectionAttempt = millis() - 10000;
  }
  else
  {
    WiFi.disconnect();
    ledFlasher.Enable();
    configMode();
  }
}

void mqttReconnect()
{
  // Loop until we're reconnected
  Serial.print("Attempting MQTT connection...");

  // Create a random client ID
  String clientId = "esp-";
  clientId += String(ESP.getChipId(), HEX);

  // Attempt to connect
  Serial.print(clientId);
  Serial.print(mqttUser);
  if (mqttClient.connect(clientId.c_str(), mqttUser.c_str(), mqttPass.c_str(), mqttGetTopic("LWT").c_str(), 1, true, "Offline"))
  {
    Serial.println("connected");
    mqttClient.subscribe(mqttGetTopic("Command").c_str());
    mqttPublish("Description", device_description.c_str());

    if (device_mode == 1 || device_mode == 2)
    {
      mqttPublish("Type", "garage");
    }

    if (device_mode == 3 || device_mode == 4)
    {
      mqttPublish("Type", "switch");
    }

    if (device_mode == 5)
    {
      mqttPublish("Type", "temperature");
    }

    if (device_mode == 6)
    {
      mqttPublish("Type", "thermostat");

      mqttPublishFloat("OnTemp", thermostatOnTemp);
      mqttPublishFloat("OffTemp", thermostatOffTemp);
    }

    if (device_mode < 5)
    {
      char tempString[2];
      sprintf(tempString, "%d", relayCount);
      mqttPublish("Relays", tempString);
    }

    handleMqttPub();
  }
  else
  {
    Serial.print("failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" try again in 10 seconds");
  }
}

void loop(void)
{

  if (infoType == INFO_TYPE_GARAGEDOOR)
  {
    if (garageDoor1.Update())
    {
      mqttPublish("Sensor1", garageDoor1.isOpen() ? "1" : "0");
    }
    if (garageDoor2.Update())
    {
      mqttPublish("Sensor2", garageDoor2.isOpen() ? "1" : "0");
    }
  }
  wifiLED.Update();
  wlStatus.Update();
  ledFlasher.Update();
  resetButton.Update();
  wirelessWatchdog.Update();
  if (inConfigMode == true)
  {
    dnsServer.processNextRequest();
  }
  else if (infoType == INFO_TYPE_THERMOSTAT)
  {
    temperature = getTemperature();

    if (temperature < thermostatOnTemp)
    {
      relayOn(1);
    }
    else if (temperature > thermostatOffTemp)
    {
      relayOff(1);
    }
    bool currentRelay1State = digitalRead(RELAY_PIN_1);
    if (lastRelayState1 != currentRelay1State)
    {
      lastRelayState1 = currentRelay1State;
      mqttPublish("Relay1", isRelayOn(1) ? "1" : "0");
    }
  }

  unsigned long currentMillis = millis();
  if (mqttEnabled && inConfigMode == false && WiFi.status() == WL_CONNECTED)
  {
    if (mqttClient.connected())
    {
      if (currentMillis - mqttLastMessage >= (mqttInterval * 1000))
      {
        mqttLastMessage = currentMillis; // Remember the time
        char message[25];
        handleMqttPub();
      }
    }
    else
    {
      if (currentMillis - mqttLastConnectionAttempt >= 10000)
      {
        mqttLastConnectionAttempt = currentMillis;
        mqttReconnect();
      }
    }
    mqttClient.loop();
  }

  server.handleClient();

  if (ESP.getFreeHeap() < 1000)
  {
    ESP.restart();
  }
}
#endif
