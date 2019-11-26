// Temperature Sensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

float getTemperature(){
  float temp;
  do {
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(0);
  } while (temp == 85.0 || temp == (-127.0));
  return temp;
}

String getTemperatureString(){
  String s;
  s = getTemperature();
  return s;
}

void handleTemperature() {
  String s;
  s = F("{ \"temperature\" : ");
  s += getTemperature();
  s += F(" }");
  server.send ( 200, JSON_CONTENT_TYPE, s );
  Serial.printf("settings heap size: %u\n", ESP.getFreeHeap());
}
