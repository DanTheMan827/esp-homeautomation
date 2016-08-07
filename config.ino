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

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  ssid = json["wap"];
  password = json["wpsk"];
  device_description = (const char*)json["desc"];
  device_mode = json["mode"];
  
  

  return true;
}

void resetConfig(){
  if ( SPIFFS.exists("/config.json") ) {
    SPIFFS.remove("/config.json");
  }
  Serial.println("Reset config");
  ESP.eraseConfig();
  ESP.reset();
}
