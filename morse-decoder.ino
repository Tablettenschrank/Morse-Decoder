// --- NEUE BIBLIOTHEKEN ---
#include <Arduino.h>
#include <U8g2lib.h> // U8g2 STATT Adafruit
#include <Wire.h>

// --- U8G2 KONSTRUKTOR ---
// WICHTIG: Dies ist die wichtigste Zeile für U8g2.
// U8G2_SH1106_128X64_NONAME_F_HW_I2C ist für viele 1.3" I2C Displays korrekt.
// Falls diese nicht geht, versuche U8G2_SSD1306_128X64_NONAME_F_HW_I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


// --- Konfiguration (unverändert) ---
const int TASTER_PIN = 23;
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

// --- Globale Variablen (unverändert) ---
int tasterStatus = HIGH;
int letzterTasterStatus = HIGH;
unsigned long drueckStartZeit = 0;
unsigned long letzteAktionZeit = 0;
String aktuelleMorseSequenz = "";
String angezeigterText = "";
bool clearAktiviert = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n--- Morse-Decoder mit U8g2 gestartet ---");

  // U8G2: Display initialisieren
  u8g2.begin(); // u8g2.begin() STATT display.begin()
  
  pinMode(TASTER_PIN, INPUT_PULLUP);

  // U8G2: Startbildschirm anzeigen
  u8g2.clearBuffer();          // Speicher leeren
  u8g2.setFont(u8g2_font_ncenB12_tr); // Schriftart setzen
  u8g2.drawStr(25, 20, "Morse"); // Text zeichnen an x, y
  u8g2.drawStr(15, 45, "Decoder");
  u8g2.sendBuffer();           // Speicher auf das Display schreiben
  delay(2000);

  updateDisplay();
}

void loop() {
  // Komplette Logik für Taster, Timing und Morse-Dekodierung ist UNVERÄNDERT
  
  tasterStatus = digitalRead(TASTER_PIN);

  if (tasterStatus == LOW && letzterTasterStatus == HIGH) {
    drueckStartZeit = millis();
    letzterTasterStatus = LOW;
  }

  if (tasterStatus == LOW && (millis() - drueckStartZeit > CLEAR_DRUCK_DAUER_MS) && !clearAktiviert) {
    Serial.println("\n>> Langes Drücken erkannt. ALLES GELÖSCHT. <<\n");
    angezeigterText = "";
    aktuelleMorseSequenz = "";
    updateDisplay();
    clearAktiviert = true;
  }

  if (tasterStatus == HIGH && letzterTasterStatus == LOW) {
    if (clearAktiviert) {
      clearAktiviert = false;
    } else {
      unsigned long drueckDauer = millis() - drueckStartZeit;
      if (aktuelleMorseSequenz.length() < MAX_SEQUENZ_LAENGE) {
        String signal = "";
        if (drueckDauer <= KURZ_DRUCK_MAX_MS) {
          aktuelleMorseSequenz += ".";
          signal = ". (kurz)";
        } else {
          aktuelleMorseSequenz += "-";
          signal = "- (lang)";
        }
        Serial.print("Signal erkannt: " + signal);
        Serial.println(" | Aktuelle Sequenz: " + aktuelleMorseSequenz);
        updateDisplay();
        letzteAktionZeit = millis();
      } else {
        Serial.println("Maximale Sequenzlänge (5) erreicht. Eingabe ignoriert.");
      }
    }
    letzterTasterStatus = HIGH;
  }

  if (aktuelleMorseSequenz.length() > 0 && (millis() - letzteAktionZeit > TIMEOUT_MS)) {
    Serial.println("\n--- Timeout ---");
    Serial.println("Dekodiere Sequenz: " + aktuelleMorseSequenz);
    char buchstabe = dekodiereMorse(aktuelleMorseSequenz);
    angezeigterText += buchstabe;
    Serial.println("Ergebnis: '" + String(buchstabe) + "'");
    Serial.println("Gesamttext: " + angezeigterText);
    Serial.println("-----------------");
    aktuelleMorseSequenz = "";
    updateDisplay();
  }
}

char dekodiereMorse(String sequenz) {
  for (int i = 0; i < sizeof(morseAlphabet) / sizeof(morseAlphabet[0]); i++) {
    if (sequenz.equals(morseAlphabet[i].code)) {
      return morseAlphabet[i].zeichen;
    }
  }
  return '?';
}

// U8G2: Angepasste Funktion zum Zeichnen des Displays
void updateDisplay() {
  // Die U8g2-Zeichen-Schleife beginnt immer mit clearBuffer()
  u8g2.clearBuffer();

  // 1. Dekodierten Text anzeigen
  u8g2.setFont(u8g2_font_logisoso16_tr); // Schöne große Schrift für das Ergebnis
  u8g2.setCursor(0, 20); // Startposition (x, y)
  // U8g2 kann Text nicht automatisch umbrechen wie Adafruit, daher manuell
  // Für dieses Projekt aber unkritisch.
  u8g2.print(angezeigterText);

  // 2. Trennlinie zeichnen
  u8g2.drawLine(0, 38, 128, 38);

  // 3. Aktuelle Morse-Eingabe anzeigen
  u8g2.setFont(u8g2_font_logisoso16_tr);
  u8g2.setCursor(0, 60);
  u8g2.print(">");
  u8g2.print(aktuelleMorseSequenz);

  // 4. ALLES auf dem Bildschirm anzeigen
  // Die Schleife endet immer mit sendBuffer()
  u8g2.sendBuffer();
}