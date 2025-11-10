#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoMqttClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h> // ArduinoJson by Benoit Blanchon
#include <time.h>
#include <limits.h>
#include "html.h"

// installed libraries:
// Adafruit SH110X Library by Adafruit
// Adafruit GFX Library by Adafruit
// DHT sensor library by Adafruit 
// Adafruit Unified Sensor by Adafruit
// ArduinoJson by Benoit Blanchon
// Adafruit BusIO by Adafruit
// ArduinoMqttClient by Arduino

struct Settings {
  String ssid = "home";
  String password = "secretPassword";
  String ssidAP = "yats";
  String mqttServer = "test.mosquitto.org";
  int mqttPort = 1883;
  String mqttTopic = "yats";
  String mqttUsername = "";
  String mqttPassword = "";
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
// MQTT support
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

unsigned long lastMqttConnectAttempt = 0;

// define intervalls for the state machine
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
  display.setContrast(64);
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
  // Try to connect to MQTT broker (if configured)
  connectToMqtt();
  
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

void loop() {
  if (checkIntervall(lastReconnectMillis, RECONNECT_INTERVALL)) {
    connectToWiFi();
    connectToMqtt();
    setClock();
  }

  if (checkIntervall(lastMeasurementMillis, MEASUREMENT_INTERVALL)) {
    if (measure() == 0) {
      setMinMax();
      // publish measurement over MQTT if connected
      publishMeasurement();
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