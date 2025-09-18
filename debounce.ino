const int buttonPin = 2;       // Pin, an dem der Taster angeschlossen ist
const int ledPin = 13;         // Pin für die LED (optional zur Anzeige)
bool buttonState = false;      // Aktueller Zustand des Tasters
bool lastButtonState = false;  // Vorheriger Zustand
unsigned long lastDebounceTime = 0;  // Zeitpunkt der letzten Änderung
unsigned long debounceDelay = 50;    // Entprellzeit in Millisekunden

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);  // Taster mit internem Pullup-Widerstand
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int reading = digitalRead(buttonPin);

  // Wenn sich der Zustand geändert hat, Zeit merken
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Wenn der Zustand stabil ist für länger als debounceDelay
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Wenn sich der Zustand geändert hat
    if (reading != buttonState) {
      buttonState = reading;

      // Aktion bei Tastendruck (LOW wegen Pullup)
      if (buttonState == LOW) {
        Serial.println("Taster gedrückt");
        digitalWrite(ledPin, !digitalRead(ledPin));  // LED toggeln
      }
    }
  }

  lastButtonState = reading;
}
