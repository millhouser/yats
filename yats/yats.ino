
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include <WiFi.h>
#include "time.h"

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

const char* ssid = "home";
const char* password = "secretpw";
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "ptbtime1.ptb.de";
const long gmtOffset_sec = 2 * 3600;
const int daylightOffset_sec = 0;

int state = 1;
int newstate = 0;
bool keys_locked = false;
unsigned long lasttime5s = 0;
unsigned long lasttime10min = 0;

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
  unsigned long currenttime = millis();

  if (state != newstate) {
    state = newstate;
  }

  switch (state) {
    case 0:
      display.clearDisplay();
      if (currenttime - lasttime10min >= 600000) {
        lasttime10min = currenttime;
        connectToWiFi();
        //printLocalTime();
        setClock();
      }

      if (currenttime - lasttime5s >= 5000) {
        lasttime5s = currenttime;
        float temp = dht.readTemperature();
        float humi = dht.readHumidity();
        if (isnan(temp) || isnan(humi)) {
          Serial.println("Lesefehler DHT22!");
        } else {
          display.setCursor(9, 0);
          display.clearDisplay();
          display.setTextSize(2);
          display.printf("%.1f", temp);
          display.print((char)247);
          display.printf("C");

          display.setCursor(87, 9);
          display.setTextSize(1);
          display.printf("%.1f%%", humi);

          display.setTextSize(1);
          display.setCursor(15, 57);

          time_t now = time(nullptr);
          struct tm timeinfo;
          gmtime_r(&now, &timeinfo);
          display.printf("%.2u.%.2u.%.4u %.2u:%.2u", timeinfo.tm_mday, timeinfo.tm_mon, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min);
          display.display();

          Serial.print("Current time: ");
          Serial.println(asctime(&timeinfo));
          Serial.printf("Temp: %.1f°C, Humi: %.1f%%\n", temp, humi);
        }
      }
      break;
    case 1:
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
    newstate = state - 1;
    if (newstate < 0) newstate = 4;
    delay(200);
  } else {
    keys_locked = false;
  }

  if (digitalRead(KEY_A) == LOW) {
    keys_locked = true;
    newstate = state + 1;
    if (newstate > 4) newstate = 0;
    delay(200);
  } else {
    keys_locked = false;
  }
}
