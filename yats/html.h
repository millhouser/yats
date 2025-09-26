// start page
const char* startPage = u8R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>yats</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="/style.css">
    <!--
    <style>
        body { font-family: Arial; margin: 40px; }
        button { padding: 10px 20px; margin: 5px; font-size: 16px; }
    </style>
    -->
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
)rawliteral";

String settingsPage = u8R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>yats - Einstellungen</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="container">
        <header>
            <h1>🔧 yats Konfiguration</h1>
            <p>Aktuelle IP: )rawliteral" + WiFi.localIP().toString() + R"rawliteral(</p>
        </header>
        
        <div class="settings-card">
            <h2>Geräteeinstellungen</h2>
            <form id="settingsForm">
                <div class="form-group">
                    <label for="ssid">SSID:</label>
                    <input type="text" id="ssid" name="ssid" required>
                </div>
                
                <div class="form-group">
                    <label for="password">Passwort:</label>
                    <input type="text" id="password" name="password" required>
                </div>
                
                <div class="form-group">
                    <label for="ssidAP">SSID Access Point Mode:</label>
                    <input type="text" id="ssidAP" name="ssidAP" required>
                </div>

                <!--
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
                -->

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
                    // document.getElementById('deviceName').value = data.deviceName;
                    // document.getElementById('refreshInterval').value = data.refreshInterval;
                    // document.getElementById('enableNotifications').checked = data.enableNotifications;
                    // document.getElementById('threshold').value = data.threshold;
                    // document.getElementById('apiKey').value = data.apiKey;
                    document.getElementById('ssid').value = data.ssid;
                    document.getElementById('password').value = data.password;
                    document.getElementById('ssidAP').value = data.ssidAP;
                })
                .catch(error => {
                    showStatus('Fehler beim Laden der Einstellungen: ' + error, 'error');
                });
        }

        document.getElementById('settingsForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const formData = new FormData(e.target);
            const settings = {};
            
            // settings.deviceName = formData.get('deviceName');
            // settings.refreshInterval = parseInt(formData.get('refreshInterval'));
            // settings.enableNotifications = formData.has('enableNotifications');
            // settings.threshold = parseFloat(formData.get('threshold'));
            // settings.apiKey = formData.get('apiKey') || '';

            settings.ssid = formData.get('ssid');
            settings.password = formData.get('password');
            settings.ssidAP = formData.get('ssidAP');

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

String css = u8R"rawliteral(
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