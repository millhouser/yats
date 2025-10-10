#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h> // ArduinoJson by Benoit Blanchon
#include <time.h>
#include <limits.h>
#include "html.h"

struct Settings {
  String ssid = "home";
  String password = "secretPassword";
  String ssidAP = "yats";
};

Settings currentSettings;

// define characters for the display
#define DEG (char)247    // '°' on the display

// configure SPI
#define OLED_MOSI 11
#define OLED_CLK 10
#define OLED_DC 8
#define OLED_CS 9
#define OLED_RST 12

// configure display
#define SCREEN_WIDTH 64  // 21 characters per line
#define SCREEN_HEIGHT 128
Adafruit_SH1107 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);

// configure DHT22
#define DHTPIN 16
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// configure keys on display
#define KEY_A 15
#define KEY_B 17
bool keyBpressed = false;

// configure web server
WebServer server(80);

// define intervalls for the state machine
#define DIM_INTERVALL 2000 // milliseconds
#define MEASUREMENT_INTERVALL 5000 // milliseconds
#define RECONNECT_INTERVALL 600000 // milliseconds

// state machine states
enum states { NORMAL,
              MINMAX_TEMP,
              MINMAX_HUMI,
              IP_ADDRESS };

// for the displayMinMax() function
enum minmax { MINMAX_MIN,
              MINMAX_MAX };

// define struct to store measurement data
struct measurement {
  time_t time = 0;
  float temp = 0.0;
  float humi = 0.0;
};

// define measurement variables
struct measurement current_measurement;
struct measurement max_temp;
struct measurement min_temp;
struct measurement max_humi;
struct measurement min_humi;

// configure NTP
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "ptbtime1.ptb.de";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// setup state machine
int state = 0;
unsigned long lastDimMillis = 0;
unsigned long lastMeasurementMillis = 0;
unsigned long lastReconnectMillis = 0;

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

  File file = LittleFS.open("/settings.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    Serial.println("Einstellungen gespeichert");
  } else {
    Serial.println("Fehler beim Speichern der Einstellungen");
  }
}

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

int measure() {
  int ret = 0;

  time_t now = time(nullptr);

  float temp = dht.readTemperature();
  float humi = dht.readHumidity();

  if (isnan(temp) || isnan(humi)) {
    Serial.println("Lesefehler DHT22!");
    return -1;
  } else {
    current_measurement.time = now;
    current_measurement.temp = temp;
    current_measurement.humi = humi;
  }

  return 0;
}

char* dateTimeStr(time_t t) {
  struct tm timeinfo;
  static char dts[15];

  //gmtime_r(&t, &timeinfo);
  localtime_r(&t, &timeinfo);

  sprintf(dts, "%.2u.%.2u.%.4u %.2u:%.2u", timeinfo.tm_mday, timeinfo.tm_mon, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min);

  return dts;
}

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

void resetMinMaxTemp() {
  min_temp = current_measurement;
  max_temp = current_measurement;
}

void resetMinMaxHumi() {
  min_humi = current_measurement;
  max_humi = current_measurement;
}

void setMinMax() {
  // set min_temp
  if ((min_temp.time == 0) || (current_measurement.temp < min_temp.temp)) {
    min_temp = current_measurement;
  }

  // set min_humi
  if ((min_humi.time == 0) || (current_measurement.humi < min_humi.humi)) {
    min_humi = current_measurement;
  }

  // set max_temp
  if ((max_temp.time == 0) || (current_measurement.temp > max_temp.temp)) {
    max_temp = current_measurement;
  }

  // set max_humi
  if ((max_humi.time == 0) || (current_measurement.humi > max_humi.humi)) {
    max_humi = current_measurement;
  }
}

bool checkIntervall(unsigned long &lastMillis, unsigned long intervall) {
  unsigned long currentMillis = millis();
  if (currentMillis < lastMillis) {
    lastMillis = intervall - (ULONG_MAX - lastMillis);
  }
  if (currentMillis - lastMillis >= intervall) {
    lastMillis = currentMillis;
    return true;
  } else {
    return false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);

  display.begin(0, true);
  display.setRotation(1);
  displaySplashScreen();

  dht.begin();

  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Fehler");
    return;
  } else {
    loadSettings();
  }

  connectToWiFi();
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  // set time zone
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);  // Mitteleuropäische Zeit
  tzset();

  setClock();
  
  resetMinMaxTemp();
  resetMinMaxHumi();

  server.on("/", handleRoot);
  server.on("/sensors", handleSensors);
  server.on("/settingspage", handleSettingsPage);
  server.on("/settings", HTTP_GET, handleGetSettings);
  server.on("/settings", HTTP_POST, handleSaveSettings);
  server.on("/style.css", handleCSS);

  server.begin();
}

uint8_t dim = 255;
void loop() {
  if (checkIntervall(lastDimMillis, DIM_INTERVALL)) {
    display.setContrast(dim);
    if (dim == 0) {
      dim = 255;
      return;
    }
    if (dim == 1) {
      dim = 0;
      return;
    }
    if (dim == 255) dim = 1;
  }

  if (checkIntervall(lastReconnectMillis, RECONNECT_INTERVALL)) {
    connectToWiFi();
    setClock();
  }

  if (checkIntervall(lastMeasurementMillis, MEASUREMENT_INTERVALL)) {
    if (measure() == 0) {
      setMinMax();
    }
  }

  switch (state) {
    case 0:  //NORMAL:
      displayMeasurement();
      break;
    case 1:  // MIN_TEMP + MAX_TEMP
      displayMinMaxTemp();
      if (keyBpressed) {
        resetMinMaxTemp();
        keyBpressed = false;
      }
      break;
    case 2:  // MIN_HUMI + MAX_HUMI
      displayMinMaxHumi();
      if (keyBpressed) {
        resetMinMaxHumi();
        keyBpressed = false;
      }
      break;
    case 3:  //IP_ADDRESS:
      displayLANInfo();
      break;
    default:
      display.clearDisplay();
      display.setCursor(0, 27);
      display.print(state);
      display.display();
  }

  if (digitalRead(KEY_A) == LOW) {
    state = state + 1;
    if (state > 3) state = 0;
    delay(200);
  } else {
  }

  if (digitalRead(KEY_B) == LOW) {
    keyBpressed = true;
    delay(200);
  } else {
    keyBpressed = false;
  }

  server.handleClient();
}