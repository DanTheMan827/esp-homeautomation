#include "FS.h"

class ConfigReset
{
  // Class Member Variables
  // These are initialized at startup
  int interval; // the duration the reset button has to be held down.
  bool isHeld = false;
  bool pin;
 
  // These maintain the current state
  unsigned long previousMillis;   // will store last time button was pushed
 
  // Constructor - creates a ConfigReset 
  // and initializes the member variables and state
  
  public:
  ConfigReset(int buttonDuration, int resetPin)
  {
    interval = buttonDuration;
    pin = resetPin;
    pinMode(pin, INPUT_PULLUP);
  }
 
  void Update()
  {
    if ( digitalRead(pin) == LOW ){
      if(isHeld == false){
        isHeld = true;
        previousMillis = millis();
        Serial.println("Reset button pressed.");
      }
    } else {
      if(isHeld == true){
        isHeld = false;
        previousMillis = millis();
        Serial.println("Reset button released.");
      }
    }
    
    if(isHeld == true && millis() - previousMillis >= interval){
      if ( SPIFFS.exists("/config.json") ) {
        SPIFFS.remove("/config.json");
      }
      Serial.println("Reset config");
      ESP.eraseConfig();
      ESP.reset();
    }
  }
};
