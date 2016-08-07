#include <Arduino.h>
class GarageDoor {
  int relayPin;
  int sensorPin;
  int pauseTime;

  bool ticking;
  unsigned long nextTrigger;

  public:
  GarageDoor(int relay, int sensor, int pause)
  {
    relayPin = relay;
    sensorPin = sensor;
    pauseTime = pause;
    ticking = false;
    nextTrigger = 0;
    pinMode(relay, OUTPUT);
    pinMode(sensor, INPUT_PULLUP);
    digitalWrite(relay, HIGH);
  }
  bool isOpen(){
    return (digitalRead(sensorPin) == HIGH);
  }
  bool openDoor(){
    if (isOpen())
      return false;
    triggerDoor();
    return true;
  }
  bool closeDoor(){
    if (!isOpen())
      return false;
    triggerDoor();
    return true;
  }
  bool triggerDoor(){
    digitalWrite(relayPin,LOW);
    nextTrigger = millis() + pauseTime;
    ticking = true;
    return true;
  }
  void Update(){
    if (ticking == true && nextTrigger < millis()){
      ticking = false;
      digitalWrite(relayPin,HIGH);
    }
  }
};

