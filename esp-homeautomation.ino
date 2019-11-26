#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "./DNSServer.h" // Patched lib
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "FS.h"

#define JSON_CONTENT_TYPE FF("text/json")
#define FALSE_STRING FF("false")
#define TRUE_STRING FF("true")

#define FF(STRING_LITERAL) ((const char*)F(STRING_LITERAL))
#define ServerOnGet(url, body) server.on(FF(url), HTTP_GET, []() { printf("HTTP GET: %s\n\n", url); body })
#define ServerSendJson(content) server.send(200, JSON_CONTENT_TYPE, content);
#define mqttIf(topic, body) if (!strncmp(topic, (const char*)payload, length)) body
