#ifndef DTM_WIFISTATUS
#define DTM_WIFISTATUS
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

class WifiStatus
{
  int currentStatus;
  String hostname;

public:
  WifiStatus()
  {
    currentStatus = 0;
    hostname = "esp-";
    hostname += String(ESP.getChipId(), HEX);
    // Set Hostname.
    WiFi.hostname(hostname);
  }

  void Update()
  {
    int newStatus = WiFi.status();
    if (newStatus != currentStatus)
    {

      currentStatus = newStatus;
      Serial.print("\r\nWi-Fi status changed to ");
      switch (newStatus)
      {
      case WL_IDLE_STATUS:
        Serial.println("IDLE");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("NO SSID AVAILABLE");
        break;
      case WL_SCAN_COMPLETED:
        Serial.println("SCAN COMPLETED");
        break;
      case WL_CONNECTED:
        Serial.println("CONNECTED");
        Serial.print("  SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("  IP address: ");
        Serial.println(WiFi.localIP());
        Serial.println("  Hostname: " + WiFi.hostname());

        if (MDNS.begin(hostname.c_str()))
        {
          Serial.println("  MDNS responder started");
        }
        MDNS.addService("node", "tcp", 80);
        MDNS.addServiceTxt("node", "tcp", "Location", "Testing 123");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("CONNECT FAILED");
        break;
      case WL_CONNECTION_LOST:
        Serial.println("CONNECTION LOST");
        break;
      case WL_DISCONNECTED:
        Serial.println("DISCONNECTED");
        break;
      default:
        break;
      }
    }
  }
};
#endif
