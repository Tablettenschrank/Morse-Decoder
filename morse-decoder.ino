// Binde die notwendigen Bibliotheken für das OLED ein
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Konfiguration ---

// Pin für den Taster
const int TASTER_PIN = 23;

// OLED Display Konfiguration
const int SCREEN_WIDTH = 128; // Pixelbreite des Displays
const int SCREEN_HEIGHT = 64;  // Pixelhöhe des Displays
const int OLED_RESET = -1;     // Reset Pin (-1, da wir den ESP32 Reset verwenden)
// I2C-Adresse des OLEDs. Meist 0x3C für 128x64 oder 1.3" Displays
const int I2C_ADRESSE = 0x3C;

// Zeit-Schwellenwerte in Millisekunden
const unsigned long KURZ_DRUCK_MAX_MS = 200;
const unsigned long LANG_DRUCK_MIN_MS = 201;
const unsigned long TIMEOUT_MS = 1500;
const unsigned long CLEAR_DRUCK_DAUER_MS = 3000;
const int MAX_SEQUENZ_LAENGE = 5;

// --- Datenstruktur für das Morse-Alphabet (unverändert) ---
struct MorseZeichen {
  char zeichen;
  const char* code;
};

MorseZeichen morseAlphabet[] = {
  {'A', ".-"},   {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},  {'E', "."},
  {'F', "..-."}, {'G', "--."},   {'H', "...."}, {'I', ".."},   {'J', ".---"},
  {'K', "-.-"},  {'L', ".-.."}, {'M', "--"},   {'N', "-."},   {'O', "---"},
  {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."},  {'S', "..."},  {'T', "-"},
  {'U', "..-"},  {'V', "...-"}, {'W', ".--"},  {'X', "-..-"}, {'Y', "-.--"},
  {'Z', "--.."},
  {'1', ".----"}, {'2', "..---"}, {'3', "...--"}, {'4', "....-"}, {'5', "....."},
  {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."}, {'0', "-----"},
  {' ', " "}
};

// --- Globale Variablen ---
// Erstelle ein Display-Objekt
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int tasterStatus = HIGH;
int letzterTasterStatus = HIGH;
unsigned long drueckStartZeit = 0;
unsigned long letzteAktionZeit = 0;
String aktuelleMorseSequenz = "";
String angezeigterText = "";
bool clearAktiviert = false;

void setup() {
  Serial.begin(1152200);
  pinMode(TASTER_PIN, INPUT_PULLUP);

  // Initialisiere das OLED-Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADRESSE)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Endlosschleife bei Fehler
  }

  // Startbildschirm anzeigen
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("Morse");
  display.setCursor(20, 35);
  display.println("Decoder");
  display.display(); // Wichtig: Änderungen anzeigen!
  delay(2000);

  updateDisplay(); // Erste Anzeige des leeren Layouts
}

void loop() {
  tasterStatus = digitalRead(TASTER_PIN);

  // 1. Taster wurde gerade GEDRÜCKT
  if (tasterStatus == LOW && letzterTasterStatus == HIGH) {
    drueckStartZeit = millis();
    letzterTasterStatus = LOW;
  }

  // 2. Logik für den Reset durch langes Drücken
  if (tasterStatus == LOW && (millis() - drueckStartZeit > CLEAR_DRUCK_DAUER_MS) && !clearAktiviert) {
    angezeigterText = "";
    aktuelleMorseSequenz = "";
    updateDisplay();
    clearAktiviert = true;
  }

  // 3. Taster wurde gerade LOSGELASSEN
  if (tasterStatus == HIGH && letzterTasterStatus == LOW) {
    if (clearAktiviert) {
      clearAktiviert = false;
    } else {
      unsigned long drueckDauer = millis() - drueckStartZeit;
      if (aktuelleMorseSequenz.length() < MAX_SEQUENZ_LAENGE) {
        if (drueckDauer <= KURZ_DRUCK_MAX_MS) {
          aktuelleMorseSequenz += ".";
        } else {
          aktuelleMorseSequenz += "-";
        }
        updateDisplay();
        letzteAktionZeit = millis();
      }
    }
    letzterTasterStatus = HIGH;
  }

  // 4. Logik zum Auswerten nach Timeout
  if (aktuelleMorseSequenz.length() > 0 && (millis() - letzteAktionZeit > TIMEOUT_MS)) {
    char buchstabe = dekodiereMorse(aktuelleMorseSequenz);
    angezeigterText += buchstabe;
    aktuelleMorseSequenz = "";
    updateDisplay();
  }
}

// Sucht die Morse-Sequenz im Alphabet und gibt das Zeichen zurück
char dekodiereMorse(String sequenz) {
  for (int i = 0; i < sizeof(morseAlphabet) / sizeof(morseAlphabet[0]); i++) {
    if (sequenz.equals(morseAlphabet[i].code)) {
      return morseAlphabet[i].zeichen;
    }
  }
  return '?';
}

// NEUE Funktion, um das OLED-Display komplett neu zu zeichnen
void updateDisplay() {
  display.clearDisplay(); // Gesamten Bildschirm leeren

  // 1. Dekodierten Text anzeigen (obere Hälfte)
  display.setTextSize(2); // Größere Schrift für das Ergebnis
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 5); // Startposition (x, y)
  display.setTextWrap(true); // Automatischer Zeilenumbruch
  display.print(angezeigterText);

  // 2. Trennlinie zeichnen
  display.drawLine(0, 38, SCREEN_WIDTH, 38, SSD1306_WHITE);

  // 3. Aktuelle Morse-Eingabe anzeigen (untere Hälfte)
  display.setTextSize(2); // Ebenfalls große Schrift
  display.setCursor(0, 45);
  display.setTextWrap(false); // Kein Umbruch hier
  display.print(">");
  display.print(aktuelleMorseSequenz);

  // 4. ALLES auf dem Bildschirm anzeigen
  display.display();
}