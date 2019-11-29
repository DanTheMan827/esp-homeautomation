#ifndef DTM_MACROS
#define DTM_MACROS
#define FF(STRING_LITERAL) ((const char *)F(STRING_LITERAL))
#define JSON_CONTENT_TYPE FF("text/json")
#define FALSE_STRING FF("false")
#define TRUE_STRING FF("true")

#define isPinOn(pin, reverse) (digitalRead(pin) == (reverse ? HIGH : LOW))
#define isRelayOn(pin) isPinOn(RELAY_PIN_##pin, RELAY_REVERSE_##pin)
#define isSensorOn(pin) isPinOn(SENSOR_PIN_##pin, SENSOR_REVERSE_##pin)
#define isDebouncerOn(pin) (debouncer##pin.update() && debouncer##pin.read() == (SENSOR_REVERSE_##pin ? HIGH : LOW))
#define relayOn(pin)                           \
  digitalWrite(RELAY_PIN_##pin, onState##pin); \
  mqttPublish("Relay" #pin, "1")

#define relayOff(pin)                           \
  digitalWrite(RELAY_PIN_##pin, offState##pin); \
  mqttPublish("Relay" #pin, "0")

#define relayToggle(pin)                                        \
  digitalWrite(RELAY_PIN_##pin, !digitalRead(RELAY_PIN_##pin)); \
  mqttPublish("Relay" #pin, isRelayOn(pin) ? "1" : "0")

#define ServerOnGet(url, body)        \
  server.on(FF(url), HTTP_GET, []() { \
    printf("HTTP GET: %s\n\n", url);  \
    body                              \
  })

#define ServerSendXxd(url, name, contentType)       \
  ServerOnGet(url, {                                \
    server.sendHeader("Content-Encoding", "gzip");  \
    server.sendHeader("Content-Type", contentType); \
    server.sendContent_P(name, name##_len);         \
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
#define mqttPublishBool(topic, value) mqttPublish(topic, value ? "1" : "0")
#endif
