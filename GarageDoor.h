#ifndef DTM_GARAGEDOOR
#define DTM_GARAGEDOOR
#include "vscodefix.h"
#include <Bounce2.h>

class GarageDoor
{
  int relayPin;
  int sensorPin;
  int pauseTime;
  bool invertRelay;
  bool invertSensor;
  Bounce debouncer = Bounce();

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
    invertRelay = false;
    invertSensor = false;
    debouncer.attach(sensor);
    debouncer.interval(200);
  }
  bool reverseRelay(bool reverse)
  {
    invertRelay = reverse;
  }
  bool reverseSensor(bool reverse)
  {
    invertSensor = reverse;
    Serial.println(reverse ? "Reverse Sensor: true" : "false");
  }
  bool isOpen()
  {
    return (debouncer.read() == (invertSensor ? LOW : HIGH));
  }
  bool openDoor()
  {
    if (isOpen())
      return false;
    triggerDoor();
    return true;
  }
  bool closeDoor()
  {
    if (!isOpen())
      return false;
    triggerDoor();
    return true;
  }
  bool triggerDoor()
  {
    digitalWrite(relayPin, (invertRelay ? HIGH : LOW));
    printf("Activate relay %d\n", relayPin);
    nextTrigger = millis() + pauseTime;
    ticking = true;
    return true;
  }
  bool read()
  {
    return debouncer.read();
  }
  bool Update()
  {
    if (ticking == true && nextTrigger < millis())
    {
      ticking = false;
      digitalWrite(relayPin, (invertRelay ? LOW : HIGH));
      printf("Deactivate relay %d\n", relayPin);
    }
    if (debouncer.update())
    {
      Serial.print("Pin ");
      Serial.print(sensorPin);
      Serial.print(" ");
      Serial.print(debouncer.read());
      Serial.print("\n");
      return true;
    }
    return false;
  }
};
#endif
