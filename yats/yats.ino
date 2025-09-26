#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <limits.h>

// configure Wifi
const char* ssid = "home";
const char* password = "HilpeTritsche";

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

// start page
const char* startPage = u8R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>yats</title>
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial; margin: 40px; }
        button { padding: 10px 20px; margin: 5px; font-size: 16px; }
    </style>
</head>
<body>
    <h1>yats Webserver</h1>
    <p>Temperatur: <span id="current_temp">--.-</span>°C</p>
    <p>Luftfeuchtigkeit: <span id="current_humi">--.-</span>%</p>
    <p>Datum/Uhrzeit: <span id="current_datetime">--.--.-- --:--</span></p>
    <br>
    <br>
    <p>Minimale Temperatur: <span id="min_temp">--.-</span>°C, <span id="min_temp_datetime">--.--.-- --:--</span></p>
    <p>Maximale Temperatur: <span id="max_temp">--.-</span>°C, <span id="max_temp_datetime">--.--.-- --:--</span></p>
    <br>
    <br>
    <p>Minimale Feuchtigkeit: <span id="min_humi">--.-</span>%, <span id="min_humi_datetime">--.--.-- --:--</span></p>
    <p>Maximale Feuchtigkeit: <span id="max_humi">--.-</span>%, <span id="max_humi_datetime">--.--.-- --:--</span></p>
    <button onclick="updateSensors()">Aktualisieren</button>

    <script>
        function updateSensors() {
            fetch('/sensors')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('current_temp').innerText = data.current_temp;
                    document.getElementById('current_humi').innerText = data.current_humi;
                    document.getElementById('current_datetime').innerText = data.current_datetime;
                    document.getElementById('min_temp').innerText = data.min_temp;
                    document.getElementById('min_temp_datetime').innerText = data.min_temp_datetime;
                    document.getElementById('max_temp').innerText = data.max_temp;
                    document.getElementById('max_temp_datetime').innerText = data.max_temp_datetime;
                    document.getElementById('min_humi').innerText = data.min_humi;
                    document.getElementById('min_humi_datetime').innerText = data.min_humi_datetime;
                    document.getElementById('max_humi').innerText = data.max_humi;
                    document.getElementById('max_humi_datetime').innerText = data.max_humi_datetime;
                });
        }
        
        // Status beim Laden der Seite aktualisieren
        updateSensors();
    </script>
</body>
</html>
)=====";

// define intervalls for the state machine
#define MEASUREMENT_INTERVALL 5000 // seconds
#define RECONNECT_INTERVALL 600000 // seconds

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
  NTP.begin(ntpServer1, ntpServer2);

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void handleRoot() {
  server.send(200, "text/html", startPage);
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

void displayIPAddress() {
  display.clearDisplay();
  display.setCursor(0, 9);
  display.print("IP-Adresse:");
  display.setCursor(0, 27);
  display.print(WiFi.localIP());
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

void setup() {
  Serial.begin(115200);
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);

  display.begin(0, true);
  display.setRotation(1);
  displaySplashScreen();

  dht.begin();

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
  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();

  // always measure
  if (currentMillis < lastMeasurementMillis) {
    lastMeasurementMillis = MEASUREMENT_INTERVALL - (ULONG_MAX - lastMeasurementMillis);
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
      displayIPAddress();
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