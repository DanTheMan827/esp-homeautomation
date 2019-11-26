#include "./DNSServer.h"                  // Patched lib
const byte        DNS_PORT = 53;          // Capture DNS requests on port 53
IPAddress         apIP(192, 168, 1, 1);    // Private network for server

void configMode() {
  inConfigMode = true;
  String wifi_ssid = "ESP ";
  wifi_ssid += String(ESP.getChipId(), HEX);
  int str_len = wifi_ssid.length() + 1;
  char char_array[str_len];
  wifi_ssid.toCharArray(char_array, str_len);

   WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(char_array);

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  server.on("/", []() {
    if ( server.hasArg("ap") ) {
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
      json["wap"] = server.arg("ap");
      json["wpsk"] = server.arg("psw");
      json["mode"] = server.arg("dt");
      json["desc"] = server.arg("desc");
      json["mqttHost"] = server.arg("mqttHost");
      json["mqttPort"] = server.arg("mqttPort").toInt();
      json["mqttUser"] = server.arg("mqttUser");
      json["mqttPass"] = server.arg("mqttPass");
      json["mqttInt"] = server.arg("mqttInt").toInt();
      json["invSense1"] = server.hasArg("invSense1");
      json["invSense2"] = server.hasArg("invSense2");
      json["invRelay1"] = server.hasArg("invRelay1");
      json["invRelay2"] = server.hasArg("invRelay2");
      json["onTemp"] = 12.2;
      json["offTemp"] = 13.3;

      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return false;
      }

      json.printTo(configFile);
      server.send(200, "text/plain", "Settings saved, restarting...");
      configFile.close();
      ESP.restart();
    } else
      handleFileRead("/config.html");
  });
  server.onNotFound ( [](){
    handleFileRead("/config.html");
  });
  server.begin();
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }
  int incomingByte;
  Serial.println("-- Config File Start --");
  while (configFile.available() > 0) {
    // read the incoming byte:
    incomingByte = configFile.read();
  
    // say what you got:
    Serial.write(incomingByte);
  }
  Serial.println("\n-- Config File End --");
  configFile.seek(0, SeekSet);

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  device_description = (const char*)json["desc"];
  ssid = (const char*)json["wap"];
  password = (const char*)json["wpsk"];
  device_mode = json["mode"];
  thermostatOnTemp = json["onTemp"];
  thermostatOffTemp = json["offTemp"];
  
  RELAY_1_REVERSE = json["invRelay1"];
  RELAY_2_REVERSE = json["invRelay2"];
  
  SENSOR_1_REVERSE = json["invSense1"];
  SENSOR_2_REVERSE = json["invSense2"];

  garageDoor1.reverseRelay(RELAY_1_REVERSE);
  garageDoor1.reverseSensor(SENSOR_1_REVERSE);

  garageDoor2.reverseRelay(RELAY_2_REVERSE);
  garageDoor2.reverseSensor(SENSOR_2_REVERSE);

  mqttHost = (const char*)json["mqttHost"];
  mqttPort = json["mqttPort"];
  mqttUser = (const char*)json["mqttUser"];
  mqttPass = (const char*)json["mqttPass"];
  mqttInterval = json["mqttInt"];

  return true;
}

void resetConfig(){
  if ( SPIFFS.exists("/config.json") ) {
    SPIFFS.remove("/config.json");
  }
  Serial.println("Reset config");
  ESP.eraseConfig();
  ESP.restart();
}
