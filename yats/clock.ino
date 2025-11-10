
void setClock() {
  NTP.begin(ntpServer1, ntpServer2);

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  int attempts = 0;
  while (now < 8 * 3600 * 2  && attempts < 10) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    attempts++;
  }
  if (now < 8 * 3600 * 2) {
    Serial.println("NTP time sync failed!");
    return;
  } else {
    Serial.println("NTP time sync successful!");
    Serial.println("");
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    Serial.print("Current time: ");
    Serial.print(asctime(&timeinfo));
  }
}
