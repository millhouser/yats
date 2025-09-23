#include <WiFi.h>
#include <WebServer.h>

// WLAN Zugangsdaten
const char* ssid = "DEIN_WLAN_NAME";
const char* password = "DEIN_WLAN_PASSWORT";

// Webserver auf Port 80
WebServer server(80);

// HTML Seite
const char* htmlPage = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Arduino Webserver</title>
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial; margin: 40px; }
        button { padding: 10px 20px; margin: 5px; font-size: 16px; }
        .on { background-color: #4CAF50; color: white; }
        .off { background-color: #f44336; color: white; }
    </style>
</head>
<body>
    <h1>Arduino Webserver</h1>
    <h2>LED Steuerung</h2>
    <p>LED Status: <span id="status">Unbekannt</span></p>
    
    <button class="on" onclick="toggleLED('on')">LED AN</button>
    <button class="off" onclick="toggleLED('off')">LED AUS</button>
    
    <h2>Sensor Werte</h2>
    <p>Temperatur: <span id="temp">--</span>°C</p>
    <button onclick="updateSensors()">Aktualisieren</button>

    <script>
        function toggleLED(state) {
            fetch('/' + state)
                .then(response => response.text())
                .then(data => {
                    document.getElementById('status').innerText = state === 'on' ? 'AN' : 'AUS';
                });
        }
        
        function updateSensors() {
            fetch('/sensors')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temp').innerText = data.temperature;
                });
        }
        
        // Status beim Laden der Seite aktualisieren
        updateSensors();
    </script>
</body>
</html>
)";

// LED Pin
const int ledPin = 2;
bool ledState = false;

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    
    // WLAN Verbindung
    WiFi.begin(ssid, password);
    Serial.print("Verbinde mit WLAN");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println();
    Serial.println("WLAN verbunden!");
    Serial.print("IP Adresse: ");
    Serial.println(WiFi.localIP());
    
    // Webserver Routen definieren
    server.on("/", handleRoot);
    server.on("/on", handleLEDOn);
    server.on("/off", handleLEDOff);
    server.on("/sensors", handleSensors);
    
    // Server starten
    server.begin();
    Serial.println("Webserver gestartet");
}

void loop() {
    server.handleClient();
}

// Handler Funktionen
void handleRoot() {
    server.send(200, "text/html", htmlPage);
}

void handleLEDOn() {
    ledState = true;
    digitalWrite(ledPin, HIGH);
    server.send(200, "text/plain", "LED ist AN");
    Serial.println("LED eingeschaltet");
}

void handleLEDOff() {
    ledState = false;
    digitalWrite(ledPin, LOW);
    server.send(200, "text/plain", "LED ist AUS");
    Serial.println("LED ausgeschaltet");
}

void handleSensors() {
    // Beispiel: Zufällige Temperatur (hier könntest du echte Sensoren auslesen)
    float temperature = 20.0 + (rand() % 100) / 10.0;
    
    String json = "{";
    json += "\"temperature\":" + String(temperature);
    json += "}";
    
    server.send(200, "application/json", json);
}