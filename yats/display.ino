
void displaySplashScreen() {
  display.clearDisplay();
  display.setTextSize(5);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(3, 6);
  display.print("yats");
  display.display();
  //  delay(1000);
}

void displayMeasurement() {
  display.clearDisplay();
  display.setCursor(9, 0);
  display.clearDisplay();
  display.setTextSize(2);
  display.printf("%.1f%cC", current_measurement.temp, DEG);

  display.setCursor(87, 9);
  display.setTextSize(1);
  display.printf("%.1f%%", current_measurement.humi);

  display.setTextSize(1);
  display.setCursor(15, 18);

  display.printf(dateTimeStr(current_measurement.time));
  display.display();
}

void displayMinMaxTemp() {
  display.clearDisplay();

  display.setCursor(0, 9);
  display.printf("min. Temp.:   %.1f%cC", min_temp.temp, DEG);
  display.setCursor(0, 18);
  display.printf(" %s", dateTimeStr(min_temp.time));

  display.setCursor(0, 36);
  display.printf("max. Temp.:   %.1f%cC", max_temp.temp, DEG);
  display.setCursor(0, 45);
  display.printf(" %s", dateTimeStr(max_temp.time));

  display.display();
}

void displayMinMaxHumi() {
  display.clearDisplay();

  display.setCursor(0, 9);
  display.printf("min. Feuchte: %.1f%%", min_humi.humi);
  display.setCursor(0, 18);
  display.printf(" %s", dateTimeStr(min_humi.time));

  display.setCursor(0, 36);
  display.printf("max. Feuchte: %.1f%%", max_humi.humi);
  display.setCursor(0, 45);
  display.printf(" %s", dateTimeStr(max_humi.time));

  display.display();
}

void displayLANInfo() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("WLAN-Informationen:");
  display.setCursor(0, 18);
  display.print("IP: " + WiFi.localIP().toString());
  display.setCursor(0, 27);
  display.print("SSID: " + String(WiFi.SSID()));
  //display.setCursor(0, 27);
  //display.print("PW: " + String(WiFi.));
  display.display();
}

void displayPower(bool powerState) {
  if (powerState) {
    if (!displayOn) {
      // implement on hardware side first
      // maybe GP12 (OLED_RST) = HIGH works good enough
      digitalWrite(OLED_RST, HIGH);
      displayOn = true;
    }
    DisplayTimeOutMillis = millis();
  } else {
    if (displayOn) {
      // implement on hardware side first
      // maybe GP12 (OLED_RST) = LOW works good enough
      digitalWrite(OLED_RST, LOW);
      displayOn = false;
    }
  }
}