#ifndef DTM_MACROS
#define DTM_MACROS
#define FF(STRING_LITERAL) ((const char *)F(STRING_LITERAL))
#define JSON_CONTENT_TYPE FF("text/json")
#define FALSE_STRING FF("false")
#define TRUE_STRING FF("true")

#define isPinOn(pin, reverse) (digitalRead(pin) == (reverse ? HIGH : LOW))
#define isRelayOn(pin) isPinOn(RELAY_PIN_##pin, RELAY_REVERSE_##pin)
#define isSensorOn(pin) isPinOn(SENSOR_PIN_##pin, SENSOR_REVERSE_##pin)
#define relayOn(pin) digitalWrite(RELAY_PIN_##pin, onState##pin)
#define relayOff(pin) digitalWrite(RELAY_PIN_##pin, offState##pin)
#define relayToggle(pin) digitalWrite(RELAY_PIN_##pin, !digitalRead(RELAY_PIN_##pin))

#define ServerOnGet(url, body)        \
  server.on(FF(url), HTTP_GET, []() { \
    printf("HTTP GET: %s\n\n", url);  \
    body                              \
  })

#define ServerSendJson(content) server.send(200, JSON_CONTENT_TYPE, content)
#define ServerSendSpiff(uri, spiff) \
  ServerOnGet(uri, {                \
    handleFileRead(spiff);          \
  })

#define mqttIf(topic, body)                           \
  if (!strncmp(topic, (const char *)payload, length)) \
  {                                                   \
    body                                              \
  }

#define mqttPublishFloat(topic, value) mqttPublish(topic, floatToString(value).c_str())
#endif
