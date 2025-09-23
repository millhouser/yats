#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

// WLAN-Zugangsdaten - hier Ihre Daten eintragen
const char* ssid = "IHR_WLAN_NAME";
const char* password = "IHR_WLAN_PASSWORT";

// NTP-Server und Zeitzone für Deutschland (UTC+1, im Sommer UTC+2)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;      // UTC+1 (Winterzeit)
const int daylightOffset_sec = 3600;  // +1 Stunde für Sommerzeit

// WiFi UDP und NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec);

// Deutsche Wochentage und Monate
const char* wochentage[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", 
                           "Donnerstag", "Freitag", "Samstag"};
const char* monate[] = {"Januar", "Februar", "März", "April", "Mai", "Juni",
                       "Juli", "August", "September", "Oktober", "November", "Dezember"};

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== Raspberry Pi Pico 2W NTP Uhr ===");
  Serial.println();
  
  // WLAN-Verbindung aufbauen
  Serial.print("Verbinde mit WLAN: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WLAN verbunden!");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  // NTP-Zeit konfigurieren
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // NTP Client starten
  timeClient.begin();
  timeClient.setTimeOffset(gmtOffset_sec + daylightOffset_sec);
  
  Serial.println("NTP-Client gestartet");
  Serial.println("Synchronisiere Zeit...");
  
  // Warten bis Zeit synchronisiert ist
  while (!timeClient.update()) {
    timeClient.forceUpdate();
    delay(1000);
  }
  
  Serial.println("Zeit erfolgreich synchronisiert!");
  Serial.println("=====================================");
  Serial.println();
}

void loop() {
  // Zeit aktualisieren
  timeClient.update();
  
  // Aktuelle Zeit holen
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Fehler beim Abrufen der Zeit");
    delay(1000);
    return;
  }
  
  // Formatierte Ausgabe
  Serial.println("=====================================");
  Serial.printf("   %s, %d. %s %d\n", 
                wochentage[timeinfo.tm_wday],
                timeinfo.tm_mday,
                monate[timeinfo.tm_mon],
                timeinfo.tm_year + 1900);
  
  Serial.printf("        %02d:%02d:%02d Uhr\n",
                timeinfo.tm_hour,
                timeinfo.tm_min,
                timeinfo.tm_sec);
  
  Serial.println("=====================================");
  Serial.println();
  
  // WLAN-Status prüfen
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WARNUNG: WLAN-Verbindung verloren!");
    Serial.println("Versuche Reconnect...");
    WiFi.reconnect();
  }
  
  // 1 Sekunde warten
  delay(1000);
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());
  
  Serial.print("Signalstärke (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}