
void handleRoot() {
  server.send(200, "text/html", startPage);
}

void handleSettingsPage() {
  server.send(200, "text/html", settingsPage);
}

void handleCSS() {
    server.send(200, "text/css", css);
}

void handleSensors() {
    String json = "{";
    json += "\"current_temp\":" + String(current_measurement.temp, 1) + ",\n";
    json += "\"current_humi\":" + String(current_measurement.humi, 1) + ",\n";
    json += "\"current_datetime\":\"" + String(dateTimeStr(current_measurement.time)) + "\",\n";
    json += "\"min_temp\":" + String(min_temp.temp, 1) + ",\n";
    json += "\"min_temp_datetime\":\"" + String(dateTimeStr(min_temp.time)) + "\",\n";
    json += "\"max_temp\":" + String(max_temp.temp, 1) + ",\n";
    json += "\"max_temp_datetime\":\"" + String(dateTimeStr(max_temp.time)) + "\",\n";
    json += "\"min_humi\":" + String(min_humi.humi, 1) + ",\n";
    json += "\"min_humi_datetime\":\"" + String(dateTimeStr(min_humi.time)) + "\",\n";
    json += "\"max_humi\":" + String(max_temp.humi, 1) + ",\n";
    json += "\"max_humi_datetime\":\"" + String(dateTimeStr(max_humi.time)) + "\"\n";
    json += "}";
    
    server.send(200, "application/json", json);
}

void handleGetSettings() {
  JsonDocument doc;
//   doc["deviceName"] = currentSettings.deviceName;
//   doc["refreshInterval"] = currentSettings.refreshInterval;
//   doc["enableNotifications"] = currentSettings.enableNotifications;
//   doc["threshold"] = currentSettings.threshold;
//   doc["apiKey"] = currentSettings.apiKey;
  doc["ssid"] = currentSettings.ssid;
  doc["password"] = currentSettings.password;
  doc["ssidAP"] = currentSettings.ssidAP;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSaveSettings() {
  if (server.hasArg("plain")) {
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    
    // Einstellungen aktualisieren
    // currentSettings.deviceName = doc["deviceName"] | currentSettings.deviceName;
    // currentSettings.refreshInterval = doc["refreshInterval"] | currentSettings.refreshInterval;
    // currentSettings.enableNotifications = doc["enableNotifications"] | currentSettings.enableNotifications;
    // currentSettings.threshold = doc["threshold"] | currentSettings.threshold;
    // currentSettings.apiKey = doc["apiKey"] | currentSettings.apiKey;
    currentSettings.ssid = doc["ssid"] | currentSettings.ssid;
    currentSettings.password = doc["password"] | currentSettings.password;
    currentSettings.ssidAP = doc["ssidAP"] | currentSettings.ssidAP;

    // In Datei speichern
    saveSettings();
    
    // Antwort senden
    JsonDocument response;
    response["success"] = true;
    response["message"] = "Einstellungen gespeichert";
    
    String responseStr;
    serializeJson(response, responseStr);
    server.send(200, "application/json", responseStr);
    
    // Debug-Ausgabe
    // Serial.println("=== Aktuelle Einstellungen ===");
    // Serial.println("Gerätename: " + currentSettings.deviceName);
    // Serial.println("Intervall: " + String(currentSettings.refreshInterval) + "s");
    // Serial.println("Benachrichtigungen: " + String(currentSettings.enableNotifications ? "An" : "Aus"));
    // Serial.println("Schwellenwert: " + String(currentSettings.threshold));
    // Serial.println("API-Key: " + (currentSettings.apiKey.length() > 0 ? "***gesetzt***" : "leer"));
    // Serial.println("==============================");
  } else {
    JsonDocument response;
    response["success"] = false;
    response["message"] = "Keine Daten erhalten";
    
    String responseStr;
    serializeJson(response, responseStr);
    server.send(400, "application/json", responseStr);
  }
}