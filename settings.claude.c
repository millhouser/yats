#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// WiFi Einstellungen - Bitte anpassen!
const char* ssid = "IHR_WIFI_NETZWERK";
const char* password = "IHR_WIFI_PASSWORT";

WebServer server(80);

// Standard-Einstellungen
struct Settings {
  String deviceName = "Pico Device";
  int refreshInterval = 10;
  bool enableNotifications = true;
  float threshold = 25.5;
  String apiKey = "";
};

Settings currentSettings;

void setup() {
  Serial.begin(115200);
  
  // LittleFS initialisieren
  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Fehler");
    return;
  }
  
  // Einstellungen laden
  loadSettings();
  
  // WiFi verbinden
  WiFi.begin(ssid, password);
  Serial.print("Verbinde mit WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("WiFi verbunden! IP-Adresse: ");
  Serial.println(WiFi.localIP());
  
  // Web-Server Routen definieren
  server.on("/", handleRoot);
  server.on("/settings", HTTP_GET, handleGetSettings);
  server.on("/settings", HTTP_POST, handleSaveSettings);
  server.on("/style.css", handleCSS);
  
  server.begin();
  Serial.println("Web-Server gestartet");
}

void loop() {
  server.handleClient();
}

void loadSettings() {
  if (LittleFS.exists("/settings.json")) {
    File file = LittleFS.open("/settings.json", "r");
    if (file) {
      String content = file.readString();
      file.close();
      
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, content);
      
      currentSettings.deviceName = doc["deviceName"] | "Pico Device";
      currentSettings.refreshInterval = doc["refreshInterval"] | 10;
      currentSettings.enableNotifications = doc["enableNotifications"] | true;
      currentSettings.threshold = doc["threshold"] | 25.5;
      currentSettings.apiKey = doc["apiKey"] | "";
      
      Serial.println("Einstellungen geladen");
    }
  } else {
    Serial.println("Keine Einstellungen gefunden, verwende Standardwerte");
    saveSettings(); // Speichere Standardwerte
  }
}

void saveSettings() {
  DynamicJsonDocument doc(1024);
  doc["deviceName"] = currentSettings.deviceName;
  doc["refreshInterval"] = currentSettings.refreshInterval;
  doc["enableNotifications"] = currentSettings.enableNotifications;
  doc["threshold"] = currentSettings.threshold;
  doc["apiKey"] = currentSettings.apiKey;
  
  File file = LittleFS.open("/settings.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    Serial.println("Einstellungen gespeichert");
  } else {
    Serial.println("Fehler beim Speichern der Einstellungen");
  }
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Raspberry Pi Pico W - Einstellungen</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="container">
        <header>
            <h1>🔧 Pico W Konfiguration</h1>
            <p>Aktuelle IP: )rawliteral" + WiFi.localIP().toString() + R"rawliteral(</p>
        </header>
        
        <div class="settings-card">
            <h2>Geräteeinstellungen</h2>
            <form id="settingsForm">
                <div class="form-group">
                    <label for="deviceName">Gerätename:</label>
                    <input type="text" id="deviceName" name="deviceName" required>
                </div>
                
                <div class="form-group">
                    <label for="refreshInterval">Aktualisierungsintervall (Sekunden):</label>
                    <input type="number" id="refreshInterval" name="refreshInterval" min="1" max="3600" required>
                </div>
                
                <div class="form-group">
                    <label class="checkbox-container">
                        <input type="checkbox" id="enableNotifications" name="enableNotifications">
                        <span class="checkmark"></span>
                        Benachrichtigungen aktivieren
                    </label>
                </div>
                
                <div class="form-group">
                    <label for="threshold">Schwellenwert:</label>
                    <input type="number" id="threshold" name="threshold" step="0.1" required>
                </div>
                
                <div class="form-group">
                    <label for="apiKey">API-Schlüssel:</label>
                    <input type="password" id="apiKey" name="apiKey" placeholder="Optional">
                </div>
                
                <button type="submit" class="btn-primary">💾 Speichern</button>
                <button type="button" class="btn-secondary" onclick="loadCurrentSettings()">🔄 Zurücksetzen</button>
            </form>
        </div>
        
        <div class="status-message" id="statusMessage"></div>
    </div>

    <script>
        // Aktuelle Einstellungen beim Laden der Seite abrufen
        window.onload = function() {
            loadCurrentSettings();
        };

        function loadCurrentSettings() {
            fetch('/settings')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('deviceName').value = data.deviceName;
                    document.getElementById('refreshInterval').value = data.refreshInterval;
                    document.getElementById('enableNotifications').checked = data.enableNotifications;
                    document.getElementById('threshold').value = data.threshold;
                    document.getElementById('apiKey').value = data.apiKey;
                })
                .catch(error => {
                    showStatus('Fehler beim Laden der Einstellungen: ' + error, 'error');
                });
        }

        document.getElementById('settingsForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const formData = new FormData(e.target);
            const settings = {};
            
            settings.deviceName = formData.get('deviceName');
            settings.refreshInterval = parseInt(formData.get('refreshInterval'));
            settings.enableNotifications = formData.has('enableNotifications');
            settings.threshold = parseFloat(formData.get('threshold'));
            settings.apiKey = formData.get('apiKey') || '';
            
            fetch('/settings', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(settings)
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showStatus('✅ Einstellungen erfolgreich gespeichert!', 'success');
                } else {
                    showStatus('❌ Fehler beim Speichern: ' + data.message, 'error');
                }
            })
            .catch(error => {
                showStatus('❌ Netzwerkfehler: ' + error, 'error');
            });
        });

        function showStatus(message, type) {
            const statusDiv = document.getElementById('statusMessage');
            statusDiv.textContent = message;
            statusDiv.className = 'status-message ' + type;
            statusDiv.style.display = 'block';
            
            setTimeout(() => {
                statusDiv.style.display = 'none';
            }, 3000);
        }
    </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleCSS() {
  String css = R"rawliteral(
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    min-height: 100vh;
    padding: 20px;
}

.container {
    max-width: 600px;
    margin: 0 auto;
}

header {
    text-align: center;
    color: white;
    margin-bottom: 30px;
}

header h1 {
    font-size: 2.5em;
    margin-bottom: 10px;
    text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
}

header p {
    opacity: 0.9;
    font-size: 1.1em;
}

.settings-card {
    background: white;
    border-radius: 15px;
    padding: 30px;
    box-shadow: 0 10px 30px rgba(0,0,0,0.2);
    backdrop-filter: blur(10px);
}

.settings-card h2 {
    color: #333;
    margin-bottom: 25px;
    text-align: center;
    font-size: 1.8em;
}

.form-group {
    margin-bottom: 20px;
}

label {
    display: block;
    margin-bottom: 8px;
    font-weight: 600;
    color: #555;
}

input[type="text"],
input[type="number"],
input[type="password"] {
    width: 100%;
    padding: 12px 15px;
    border: 2px solid #e1e5e9;
    border-radius: 8px;
    font-size: 16px;
    transition: border-color 0.3s ease;
}

input[type="text"]:focus,
input[type="number"]:focus,
input[type="password"]:focus {
    outline: none;
    border-color: #667eea;
    box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
}

.checkbox-container {
    display: flex;
    align-items: center;
    cursor: pointer;
    font-weight: 600;
}

.checkbox-container input {
    margin-right: 10px;
    transform: scale(1.2);
}

.btn-primary,
.btn-secondary {
    padding: 12px 24px;
    border: none;
    border-radius: 8px;
    font-size: 16px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.3s ease;
    margin-right: 10px;
    margin-top: 10px;
}

.btn-primary {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
}

.btn-primary:hover {
    transform: translateY(-2px);
    box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
}

.btn-secondary {
    background: #f8f9fa;
    color: #333;
    border: 2px solid #e1e5e9;
}

.btn-secondary:hover {
    background: #e9ecef;
    transform: translateY(-1px);
}

.status-message {
    margin-top: 20px;
    padding: 15px;
    border-radius: 8px;
    text-align: center;
    font-weight: 600;
    display: none;
}

.status-message.success {
    background: #d4edda;
    color: #155724;
    border: 1px solid #c3e6cb;
}

.status-message.error {
    background: #f8d7da;
    color: #721c24;
    border: 1px solid #f5c6cb;
}

@media (max-width: 768px) {
    .container {
        padding: 10px;
    }
    
    .settings-card {
        padding: 20px;
    }
    
    header h1 {
        font-size: 2em;
    }
}
)rawliteral";
  
  server.send(200, "text/css", css);
}

void handleGetSettings() {
  DynamicJsonDocument doc(1024);
  doc["deviceName"] = currentSettings.deviceName;
  doc["refreshInterval"] = currentSettings.refreshInterval;
  doc["enableNotifications"] = currentSettings.enableNotifications;
  doc["threshold"] = currentSettings.threshold;
  doc["apiKey"] = currentSettings.apiKey;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSaveSettings() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    
    // Einstellungen aktualisieren
    currentSettings.deviceName = doc["deviceName"] | currentSettings.deviceName;
    currentSettings.refreshInterval = doc["refreshInterval"] | currentSettings.refreshInterval;
    currentSettings.enableNotifications = doc["enableNotifications"] | currentSettings.enableNotifications;
    currentSettings.threshold = doc["threshold"] | currentSettings.threshold;
    currentSettings.apiKey = doc["apiKey"] | currentSettings.apiKey;
    
    // In Datei speichern
    saveSettings();
    
    // Antwort senden
    DynamicJsonDocument response(256);
    response["success"] = true;
    response["message"] = "Einstellungen gespeichert";
    
    String responseStr;
    serializeJson(response, responseStr);
    server.send(200, "application/json", responseStr);
    
    // Debug-Ausgabe
    Serial.println("=== Aktuelle Einstellungen ===");
    Serial.println("Gerätename: " + currentSettings.deviceName);
    Serial.println("Intervall: " + String(currentSettings.refreshInterval) + "s");
    Serial.println("Benachrichtigungen: " + String(currentSettings.enableNotifications ? "An" : "Aus"));
    Serial.println("Schwellenwert: " + String(currentSettings.threshold));
    Serial.println("API-Key: " + (currentSettings.apiKey.length() > 0 ? "***gesetzt***" : "leer"));
    Serial.println("==============================");
  } else {
    DynamicJsonDocument response(256);
    response["success"] = false;
    response["message"] = "Keine Daten erhalten";
    
    String responseStr;
    serializeJson(response, responseStr);
    server.send(400, "application/json", responseStr);
  }
}