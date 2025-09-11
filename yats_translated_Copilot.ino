
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <time.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 16
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define KEY_A 15
#define KEY_B 17

const char* ssid = "home";
const char* password = "HilpeTritsche";
const char* ntpServer = "pool.ntp.org";
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

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.printf("Zeit gesetzt: %02d:%02d:%02d
", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void setup() {
  Serial.begin(115200);
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);

  dht.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 27);
  display.print("yats");
  display.display();
  delay(1000);

  connectToWiFi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  unsigned long currenttime = millis();

  if (state != newstate) {
    state = newstate;
  }

  if (state == 0) {
    display.clearDisplay();
    if (currenttime - lasttime10min >= 600000) {
      lasttime10min = currenttime;
      connectToWiFi();
      printLocalTime();
    }

    if (currenttime - lasttime5s >= 5000) {
      lasttime5s = currenttime;
      float temp = dht.readTemperature();
      float humi = dht.readHumidity();
      if (isnan(temp) || isnan(humi)) {
        Serial.println("Lesefehler DHT22!");
      } else {
        display.setCursor(0, 27);
        display.clearDisplay();
        display.printf("%.1f°C %.1f%%", temp, humi);
        display.display();
        Serial.printf("Temp: %.1f°C, Humi: %.1f%%
", temp, humi);
      }
    }
  } else {
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
