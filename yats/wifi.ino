
void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(currentSettings.ssid.c_str(), currentSettings.password.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
  } else {
    Serial.println("WiFi connection failed, starting access point mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(currentSettings.ssidAP.c_str());
    Serial.println("Access point started");
  }
}