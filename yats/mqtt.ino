

// MQTT connection helper
void connectToMqtt() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqttClient.connected()) return;

  String clientId = "yats-" + WiFi.localIP().toString();
  clientId.replace('.', '_');

  Serial.print("Connecting to MQTT broker ");
  Serial.print(currentSettings.mqttServer);
  Serial.print(":");
  Serial.println(currentSettings.mqttPort);

  // prepare username/password pointers (nullptr if empty)
  const char* user = currentSettings.mqttUsername.length() ? currentSettings.mqttUsername.c_str() : nullptr;
  const char* pass = currentSettings.mqttPassword.length() ? currentSettings.mqttPassword.c_str() : nullptr;

  mqttClient.setUsernamePassword(user, pass);

  // connect using optional credentials (clientId handled by library or left default)
  bool ok = mqttClient.connect(currentSettings.mqttServer.c_str(), currentSettings.mqttPort); //, user, pass);
  if (ok) {
    Serial.println("MQTT connected");
  } else {
    Serial.println("MQTT connection failed");
  }
}

void publishMeasurement() {
  if (!mqttClient.connected()) return;

  String topic = currentSettings.mqttTopic + "/telemetry";
  String payload = "{";
  payload += "\"temperature\":" + String(current_measurement.temp, 1) + ",";
  payload += "\"humidity\":" + String(current_measurement.humi, 1) + ",";
  payload += "\"time\":\"" + String(dateTimeStr(current_measurement.time)) + "\"";
  payload += "}";

  // ArduinoMqttClient uses beginMessage/print/endMessage pattern
  if (mqttClient.beginMessage(topic.c_str())) {
    mqttClient.print(payload);
    bool ok = mqttClient.endMessage();
    
    if (ok) {
      Serial.print("Published to "); Serial.print(topic); Serial.print(": "); Serial.println(payload);
    } else {
      Serial.println("MQTT publish failed");
    }
  } else {
    Serial.println("MQTT begin message failed");
  }
}