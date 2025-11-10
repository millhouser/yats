
void loadSettings() {
  if (LittleFS.exists("/settings.json")) {
    File file = LittleFS.open("/settings.json", "r");
    if (file) {
      String content = file.readString();
      file.close();
      
      JsonDocument doc;
      deserializeJson(doc, content);
      
      // currentSettings.deviceName = doc["deviceName"] | "Pico Device";
      // currentSettings.refreshInterval = doc["refreshInterval"] | 10;
      // currentSettings.enableNotifications = doc["enableNotifications"] | true;
      // currentSettings.threshold = doc["threshold"] | 25.5;
      // currentSettings.apiKey = doc["apiKey"] | "";
      currentSettings.ssid = doc["ssid"] | "home";
      currentSettings.password = doc["password"] | "secretPassword";
      currentSettings.ssidAP = doc["ssidAP"] | "yats";
      currentSettings.mqttServer = doc["mqttServer"] | "test.mosquitto.org";
      currentSettings.mqttPort = doc["mqttPort"] | 1883;
      currentSettings.mqttTopic = doc["mqttTopic"] | "yats";
      currentSettings.mqttUsername = doc["mqttUsername"] | "";
      currentSettings.mqttPassword = doc["mqttPassword"] | "";
      Serial.println("Einstellungen geladen");
    }
  } else {
    Serial.println("Keine Einstellungen gefunden, verwende Standardwerte");
    saveSettings(); // Speichere Standardwerte
  }
}

void saveSettings() {
  JsonDocument doc;
  // doc["deviceName"] = currentSettings.deviceName;
  // doc["refreshInterval"] = currentSettings.refreshInterval;
  // doc["enableNotifications"] = currentSettings.enableNotifications;
  // doc["threshold"] = currentSettings.threshold;
  // doc["apiKey"] = currentSettings.apiKey;
  doc["ssid"] = currentSettings.ssid;
  doc["password"] = currentSettings.password;
  doc["ssidAP"] = currentSettings.ssidAP;
  doc["mqttServer"] = currentSettings.mqttServer;
  doc["mqttPort"] = currentSettings.mqttPort;
  doc["mqttTopic"] = currentSettings.mqttTopic;
  doc["mqttUsername"] = currentSettings.mqttUsername;
  doc["mqttPassword"] = currentSettings.mqttPassword;

  File file = LittleFS.open("/settings.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    Serial.println("Einstellungen gespeichert");
  } else {
    Serial.println("Fehler beim Speichern der Einstellungen");
  }
}