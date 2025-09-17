#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include <WiFi.h>
#include "time.h"
#include <limits.h>

#define SCREEN_WIDTH   64
#define SCREEN_HEIGHT 128
#define OLED_MOSI     11
#define OLED_CLK      10
#define OLED_DC       8
#define OLED_CS       9
#define OLED_RST      12
Adafruit_SH1107 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);

#define DHTPIN 16
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define KEY_A 15
#define KEY_B 17

#define MEASUREMENT_INTERVALL   5000
#define RECONNECT_INTERVALL   600000

enum states { NORMAL, MINMAX_TEMP, MINMAX_HUMI, IP_ADDRESS};

struct measurement {
  time_t time = 0;
  float temp = 0.0;
  float humi = 0.0;
};
struct measurement current_measurement;
struct measurement max_temp;
struct measurement min_temp;
struct measurement max_humi;
struct measurement min_humi;

const char* ssid = "home";
const char* password = "secretPassword";
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "ptbtime1.ptb.de";
//const long gmtOffset_sec = 2 * 3600;
//const int daylightOffset_sec = 0;

int state = 0;
bool keys_locked = false;
unsigned long lastMeasurementMillis = 0;
unsigned long lastReconnectMillis = 0;

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
  } else {
    Serial.println("WiFi connection failed");
  }
}

void setClock() {
  NTP.begin("pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
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

char * dateTimeStr(time_t t) {
  struct tm timeinfo;
  static char dts[15];

  gmtime_r(&t, &timeinfo);

  sprintf(dts, "%.2u.%.2u.%.4u %.2u:%.2u", timeinfo.tm_mday, timeinfo.tm_mon, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min);

  return dts;

}

void displayMeasurement() {
  display.setCursor(9, 0);
  display.clearDisplay();
  display.setTextSize(2);
  display.printf("%.1f", current_measurement.temp);
  display.print((char)247);
  display.printf("C");

  display.setCursor(87, 9);
  display.setTextSize(1);
  display.printf("%.1f%%", current_measurement.humi);

  display.setTextSize(1);
  display.setCursor(15, 57);

  display.printf(dateTimeStr(current_measurement.time));
  display.display();
}

void setMinMax() {
  // set max_temp
  if ((max_temp.time == 0) || (current_measurement.temp > max_temp.temp)) {
    max_temp = current_measurement;
  }

  // set min_temp
  if ((min_temp.time == 0) || (current_measurement.temp < min_temp.temp)) {
    min_temp = current_measurement;
  }

  // set max_humi
  if ((max_humi.time == 0) || (current_measurement.humi > max_humi.humi)) {
    max_humi = current_measurement;
  }

  // set min_humi
  if ((min_humi.time == 0) || (current_measurement.humi < min_humi.humi)) {
    min_humi = current_measurement;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);

  dht.begin();

  display.begin(0, true);
  display.setRotation(1);
  display.clearDisplay();
  display.setTextSize(5);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(3, 6);
  display.print("yats");
  display.display();
  delay(1000);
  display.setTextSize(4);

  connectToWiFi();
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  setClock();
}

void loop() {
  unsigned long currentMillis = millis();

  // always measure
  if (currentMillis < lastMeasurementMillis) {
    lastMeasurementMillis = MEASUREMENT_INTERVALL -( ULONG_MAX - lastMeasurementMillis);
  }
  if (currentMillis < lastReconnectMillis) {
    lastMeasurementMillis = RECONNECT_INTERVALL - (ULONG_MAX - lastReconnectMillis);
  }

  if (currentMillis - lastReconnectMillis >= RECONNECT_INTERVALL) {
    lastReconnectMillis = currentMillis;
    connectToWiFi();
    setClock();
  }

  if (currentMillis - lastMeasurementMillis >= MEASUREMENT_INTERVALL) {
    lastMeasurementMillis = currentMillis;

    if (measure() == 0) {
      setMinMax();
    }
  }

  switch (state) {
    case 0: //NORMAL:
      display.clearDisplay();
      displayMeasurement();

      break;
    case 1: //MAX_TEMP + MAX_HUMI
      display.clearDisplay();
      display.setCursor(0, 0);
      display.printf("max. Temp.:   %.1f", max_temp.temp);
      display.setCursor(0, 9);
      display.printf(" %s", dateTimeStr(max_temp.time));
      display.setCursor(0, 18);
      display.printf("max. Feuchte: %.1f", max_humi.humi);
      display.setCursor(0, 27);
      display.printf(" %s", dateTimeStr(max_humi.time));
      display.display();
      break;
    case 2: //MIN_TEMP + MIN_HUMI
      display.clearDisplay();
      display.setCursor(0, 0);
      display.printf("min. Temp.:   %.1f", min_temp.temp);
      display.setCursor(0, 9);
      display.printf(" %s", dateTimeStr(min_temp.time));
      display.setCursor(0, 18);
      display.printf("min. Feuchte: %.1f", min_humi.humi);
      display.setCursor(0, 27);
      display.printf(" %s", dateTimeStr(min_humi.time));
      display.display();
      break;
    case 3: //IP_ADDRESS:
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("IP-Adresse:");
      display.setCursor(0, 9);
      display.print(WiFi.localIP());
      display.display();
      break;
    default:
      display.clearDisplay();
      display.setCursor(0, 27);
      display.print(state);
      display.display();
  }

  if (digitalRead(KEY_B) == LOW) {
    keys_locked = true;
    state = state - 1;
    if (state < 0) state = 4;
    delay(200);
  } else {
    keys_locked = false;
  }

  if (digitalRead(KEY_A) == LOW) {
    keys_locked = true;
    state = state + 1;
    if (state > 4) state = 0;
    delay(200);
  } else {
    keys_locked = false;
  }
}