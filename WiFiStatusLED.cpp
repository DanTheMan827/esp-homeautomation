#include <Arduino.h>
#include <ESP8266WiFi.h>
class WiFiStatusLED {
  // Class member variables
  // These are initialized at startup

  int ledPin;   // the LED pin number
  int ledDuration;  // milliseconds of on and off time
  int ledPause; // pause between status flashes
  
  // These maintain the current state
  int ledState;                  // ledState used to set the LED
  unsigned long previousMillis;  // will store the last time the LED was updated
  int currentStatus;
  int currentIncrement;
  bool enabled;
  unsigned long nextMillis;
  
  // Constructor - creates the flasher
  // and initialized the member variables and state
  public:
  WiFiStatusLED(int pin, long duration, long pause){
    ledPin = pin;
    ledDuration = duration;
    ledPause = pause;

    pinMode(ledPin, OUTPUT);

    ledState = HIGH;
    previousMillis = 0;
    currentIncrement = 0;
    currentStatus = 0;
    enabled = false;
  }

  void Enable(){
    currentStatus = WiFi.status();
    enabled = true;
    currentIncrement = 1;
    nextMillis = millis();
  }

  void Disable(){
    enabled = false;
  }
  
  void Update(){
    if (enabled == true) {
      // check to see if it's time to change the state of the LED
      unsigned long currentMillis = millis();
  
      if (currentMillis > nextMillis){
        
        nextMillis = currentMillis + ledDuration;
        
        if (ledState == HIGH && currentStatus > 0) {
          ledState = LOW;
          
        } else {
          currentIncrement += 1;
          ledState = HIGH;
          if (currentIncrement > currentStatus) {
            currentIncrement = 1;
            currentStatus = WiFi.status();
            nextMillis = currentMillis + ledPause;
          }
        }
  
        digitalWrite(ledPin, ledState);
        
      }
    }
  }
};
