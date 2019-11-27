#ifndef DTM_HTTPINFORMER
#define DTM_HTTPINFORMER
#include <Arduino.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>

class HTTPInformer
{
  // Class Member Variables
  // These are initialized at startup
  int interval; // the number of milliseconds between updates
  String reqHost;
  String reqPath;
  bool enabled = false;
  WiFiClient client;

  // These maintain the current state
  unsigned long previousMillis; // will store last time LED was updated

  // Constructor - creates a HTTPInformer
  // and initializes the member variables and state

public:
  HTTPInformer(int timebetween, String host, String path)
  {
    reqHost = host;
    reqPath = path;
    interval = timebetween * 1000;
    previousMillis = millis() - interval;
  }

  void Enable()
  {
    enabled = true;
  }
  void Disable()
  {
    enabled = false;
  }

  bool ShouldRun()
  {
    unsigned long currentMillis = millis();
    return enabled == true && (currentMillis - previousMillis >= interval) && WiFi.status() == WL_CONNECTED;
  }

  void Update(String PostData, bool force = false)
  {
    if (enabled == true && WiFi.status() == WL_CONNECTED)
    {
      // check to see if it's time to change the state of the LED
      unsigned long currentMillis = millis();

      if (force == true || (currentMillis - previousMillis >= interval))
      {
        previousMillis = currentMillis; // Remember the time

        if (client.connect(reqHost.c_str(), 80))
        {

          client.print("POST ");
          client.print(reqPath);
          client.println(" HTTP/1.1");
          client.print("Host: ");
          client.println(reqHost);
          client.println("Cache-Control: no-cache");
          client.println("Content-Type: application/x-www-form-urlencoded");
          client.print("Content-Length: ");
          client.println(PostData.length());
          client.println();
          client.println(PostData);

          long intTimeout = 2000;
          unsigned long intCurrMillis = millis(), intPrevMillis = millis();

          while (!client.available())
          {

            if ((intCurrMillis - intPrevMillis) > intTimeout)
            {

              Serial.println("Timeout");
              client.stop();
              return;
            }
            intCurrMillis = millis();
          }

          while (client.connected())
          {
            if (client.available())
            {
              char str = client.read();
              Serial.print(str);
            }
          }
        }
      }
    }
  }
};
#endif
