#ifndef DTM_TEMPERATURE
#define DTM_TEMPERATURE
#include "./vscodefix.h"
#include <Arduino.h>
#include <pins_arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "./Main.h"

// Temperature Sensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

float getTemperature()
{
  float temp;
  do
  {
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(0);
  } while (temp == 85.0 || temp == (-127.0));
  return temp;
}

void handleTemperature()
{
  String s;
  s = F("{ \"temperature\" : ");
  s += getTemperature();
  s += F(" }");
  server.send(200, JSON_CONTENT_TYPE, s);
  Serial.printf("settings heap size: %u\n", ESP.getFreeHeap());
}
#endif
