# esp-homeautomation

This is an arduino sketch for ESP-8266 based boards to enable various home automation tasks

## Features

- Control 1 or 2 garage doors with a relay and an optional sensor
- Monitor a DS18B20 temperature sensor
- Control 1 or 2 relays
- Basic thermostat functionality with a DS18B20 temperature sensor and a relay
- Monitoring and control over MQTT

## Pre-requisites

- [Arduino IDE](https://www.arduino.cc/en/Main/Software)
- [ESP-8266 Board Support](https://github.com/esp8266/Arduino)
- [ESP8266FS](https://github.com/esp8266/arduino-esp8266fs-plugin)

## Connection (For NodeMCU boards)

**For Relays**

- Relay 1 input connects to D0
- Relay 2 input connects to D1

**For Garage Doors**

- Relay 1 input connects to D0
- Relay 2 input connects to D1
- Sensor 1 connects to D2
- Sensor 2 connects to D6

**For DS18B20 temperature sensor**

- 3.3V and ground connect to their corresponding pins
- Data connects to D5

**For Thermostat with DS18B20 Temperature Sensor**

- 3.3V and ground connect to their corresponding pins
- Data connects to D5
- Relay 1 input connects to D0

## NodeMCU Pin Definition

![NodeMCU Pin Definition](https://raw.githubusercontent.com/nodemcu/nodemcu-devkit-v1.0/a767ef1fd218cce4566b4414e5b6c0292d05c12f/Documents/NODEMCU_DEVKIT_V1.0_PINMAP.png)
