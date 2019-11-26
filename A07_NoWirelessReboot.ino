class NoWirelessReboot
{
    // Class Member Variables
    // These are initialized at startup
    int interval; // the duration the reset button has to be held down.
    bool isConnected;
    bool enabled;

    // These maintain the current state
    unsigned long previousMillis;   // will store last time button was pushed

  public:
    NoWirelessReboot(int noConnectionDuration)
    {
      interval = noConnectionDuration;
      previousMillis = millis();
      enabled = false;
    }

    void Enable()
    {
      enabled = true;
      previousMillis = millis();
    }

    void Disable()
    {
      enabled = false;
      previousMillis = millis();
    }

    void Update()
    {
      if (WiFi.status() == WL_CONNECTED) {
        previousMillis = millis();
        if (isConnected == false) {
          isConnected = true;
          printf("Wireless Connected\n");
        }
      } else {
        if (isConnected == true) {
          isConnected = false;
          previousMillis = millis();
          printf("Wireless Disconnected\n");
        }
      }

      if (enabled && isConnected == false && millis() - previousMillis >= interval) {
        ESP.restart();
      }
    }
};
