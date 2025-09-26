// ---------- Konfiguration ----------
#include <WiFi.h>                       // Pico W/2W WLAN (Philhower Core)
#include <LittleFS.h>                   // Internes Flash-Dateisystem
#include <AsyncWebServer_RP2040W.h>     // Asynchroner HTTP-Server (RP2040W)

// >>>> HIER DEIN WLAN EINTRAGEN <<<<
const char* WIFI_SSID = "DEIN_SSID";
const char* WIFI_PASS = "DEIN_PASS";

static const char* CONFIG_PATH = "/config.json";

// Beispiel-Configstruktur
struct Config {
  String deviceName   = "Pico2W";
  int    threshold    = 42;
  bool   enabled      = true;
  int    sampleMs     = 1000;
} cfg;

AsyncWebServer server(80);

// Hilfs: Boolean-Parsing
bool toBool(const String& s) {
  String t = s; t.toLowerCase();
  return (t == "1" || t == "true" || t == "on" || t == "yes");
}

// Config laden
bool loadConfig() {
  if (!LittleFS.exists(CONFIG_PATH)) return false;
  File f = LittleFS.open(CONFIG_PATH, "r");
  if (!f) return false;

  // Ganz einfache JSON-Extraktion ohne externe Lib
  // Erwartet {"deviceName":"..","threshold":123,"enabled":true,"sampleMs":1000}
  String json = f.readString();
  f.close();

  // Grob & robust (für Demo). Für komplexere Formate lieber ArduinoJson nutzen.
  auto grabStr = [&](const char* key,.indexOf(String("\"") + key + "\"");
    if (k < 0) return;
    int c = json.indexOf(':', k);
    int q1 = json.indexOf('\"', c + 1);
    int q2 = json.indexOf('\"', q1 + 1);
    if (q1 >= 0 && q2 > q1) out = json.substring(q1 + 1, q2);
  };
  auto grabInt = & {
    int k = json.indexOf(String("\"") + key + "\"");
    if (k < 0) return;
    int c = json.indexOf(':', k);
    int e = json.indexOf(',', c + 1);
    if (e < 0) e = json.indexOf('}', c + 1);
    if (e > c) out = json.substring(c + 1, e).toInt();
  };
  auto grabBool = & {
    int k = json.indexOf(String("\"") + key + "\"");
    if (k < 0) return;
    int c = json.indexOf(':', k);
    int e = json.indexOf(',', c + 1);
    if (e < 0) e = json.indexOf('}', c + 1);
    if (e > c) {
      String v = json.substring(c + 1, e); v.trim(); v.toLowerCase();
      out = (v.startsWith("true") || v == "1");
    }
  };

  grabStr ("deviceName", cfg.deviceName);
  grabInt ("threshold",  cfg.threshold);
  grabBool("enabled",    cfg.enabled);
  grabInt ("sampleMs",   cfg.sampleMs);

  return true;
}

// Config speichern (als JSON)
bool saveConfig() {
  File f = LittleFS.open(CONFIG_PATH, "w");
  if (!f) return false;
  String json = "{";
  json += "\"deviceName\":\"" + cfg.deviceName + "\",";
  json += "\"threshold\":"  + String(cfg.threshold) + ",";
  json += "\"enabled\":"    + String(cfg.enabled ? "true" : "false") + ",";
  json += "\"sampleMs\":"   + String(cfg.sampleMs);
  json += "}";
  f.print(json);
  f.close();
  return true;
}

// Einfache HTML-Oberfläche (ohne externe Dateien)
const char index_html[] PROGMEM = R"HTML(
<!doctype html><html lang="de"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Pico 2W – Einstellungen</title>
<style>
body{font-family:system-ui,-apple-system,Segoe UI,Roboto;max-width:720px;margin:2rem auto;padding:0 1rem}
h1{font-size:1.25rem}
fieldset{border:1px solid #ccc;padding:1rem;border-radius:.5rem}
label{display:block;margin:.5rem 0 .25rem}
input[type=text],input[type=number]{width:100%;padding:.5rem;border:1px solid #bbb;border-radius:.25rem}
.switch{display:flex;align-items:center;gap:.5rem}
button{margin-top:1rem;padding:.6rem 1rem;border:0;border-radius:.4rem;background:#0067b8;color:#fff;cursor:pointer}
#status{margin-top:.75rem}
</style>
</head><body>
<h1>Pico 2W – Einstellungen</h1>
<fieldset>
  <label for="deviceName">Gerätename</label>
  <input id="deviceName" type="text" required>
  <label for="threshold">Schwellwert</label>
  <input id="threshold" type="number" step="1" min="0" max="100000">
  <div class="switch">
    <input id="enabled" type="checkbox"><label for="enabled">Aktiviert</label>
  </div>
  <label for="sampleMs">Sample-Intervall [ms]</label>
  <input id="sampleMs" type="number" step="1" min="1" max="600000">
  <button id="saveBtn">Speichern</button>
  <div id="status"></div>
</fieldset>

<script>
async function loadCfg(){
  try{
    const r=await fetch('/api/config'); if(!r.ok) throw 0;
    const j=await r.json();
    deviceName.value=j.deviceName??'';
    threshold.value=Number(j.threshold??0);
    enabled.checked=!!j.enabled;
    sampleMs.value=Number(j.sampleMs??1000);
  }catch(e){status.textContent='Konnte Konfiguration nicht laden.';}
}
async function saveCfg(){
  const data=new URLSearchParams();
  data.set('deviceName', deviceName.value);
  data.set('threshold', threshold.value);
  data.set('enabled', enabled.checked ? '1':'0');
  data.set('sampleMs', sampleMs.value);
  try{
    const r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:data});
    status.textContent = r.ok ? 'Gespeichert.' : 'Fehler beim Speichern.';
  }catch(e){status.textContent='Netzwerkfehler.';}
}
document.getElementById('saveBtn').addEventListener('click', saveCfg);
loadCfg();
</script>
</body></html>
)HTML";

void setup() {
  Serial.begin(115200);
  delay(200);

  // LittleFS starten (bei Fehler optional formatieren)
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount fehlgeschlagen, formatiere...");
    LittleFS.format();
    LittleFS.begin();
  }

  // Defaults -> falls keine Datei vorhanden
  if (!loadConfig()) {
    saveConfig();
  }

  // WLAN verbinden
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Verbinde mit WLAN");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries++ < 60) { // ~30s Timeout
    delay(500); Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Verbunden. IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("WLAN-Verbindung fehlgeschlagen. Starte trotzdem Webserver (AP-Feature optional).");
  }

  // Routen
  server.on("/", HTTP_GET, {
    req->send_P(200, "text/html; charset=utf-8", index_html);
  });

  // Aktuelle Konfiguration (JSON)
  server.on("/api/config", HTTP_GET, [](AsyncWebServer = "{";
    json += "\"deviceName\":\"" + cfg.deviceName + "\",";
    json += "\"threshold\":"  + String(cfg.threshold) + ",";
    json += "\"enabled\":"    + String(cfg.enabled ? "true" : "false") + ",";
    json += "\"sampleMs\":"   + String(cfg.sampleMs);
    json += "}";
    req->send(200, "application/json; charset=utf-8", json);
  });

  // Speichern (POST, x-www-form-urlencoded)
  server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *eviceName", true)) cfg.deviceName = req->getParam("deviceName", true)->value();
    if (req->hasParam("threshold",  true)) cfg.threshold  = req->getParam("threshold",  true)->value().toInt();
    if (req->hasParam("enabled",    true)) cfg.enabled    = toBool(req->getParam("enabled", true)->value());
    if (req->hasParam("sampleMs",   true)) cfg.sampleMs   = req->getParam("sampleMs",   true)->value().toInt();

    bool ok = saveConfig();
    req->send(ok ? 200 : 500, "text/plain; charset=utf-8", ok ? "OK" : "ERR");
  });

  server.begin();
}

void loop() {
  // Nichts erforderlich dank Async-Server
}
