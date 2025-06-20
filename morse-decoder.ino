// Binde die notwendigen Bibliotheken für das OLED ein
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Konfiguration ---
const int TASTER_PIN = 23;
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET = -1;
const int I2C_ADRESSE = 0x3C;
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
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int tasterStatus = HIGH;
int letzterTasterStatus = HIGH;
unsigned long drueckStartZeit = 0;
unsigned long letzteAktionZeit = 0;
String aktuelleMorseSequenz = "";
String angezeigterText = "";
bool clearAktiviert = false;

void setup() {
  Serial.begin(115200); // Startet die serielle Kommunikation
  // SERIELLE AUSGABE: Startnachricht
  Serial.println("\n\n--- Morse-Decoder gestartet ---");
  Serial.println("Warte auf Eingabe...");
  
  pinMode(TASTER_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADRESSE)) {
    Serial.println(F("Fehler: SSD1306 Display konnte nicht initialisiert werden."));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("Morse");
  display.setCursor(20, 35);
  display.println("Decoder");
  display.display();
  delay(2000);

  updateDisplay();
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
    // SERIELLE AUSGABE: Bestätigung für das Löschen
    Serial.println("\n>> Langes Drücken erkannt. ALLES GELÖSCHT. <<\n");
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
        String signal = ""; // Temporärer String für die Ausgabe
        if (drueckDauer <= KURZ_DRUCK_MAX_MS) {
          aktuelleMorseSequenz += ".";
          signal = ". (kurz)";
        } else {
          aktuelleMorseSequenz += "-";
          signal = "- (lang)";
        }
        // SERIELLE AUSGABE: Welches Signal erkannt wurde und wie die Sequenz jetzt aussieht
        Serial.print("Signal erkannt: " + signal);
        Serial.println(" | Aktuelle Sequenz: " + aktuelleMorseSequenz);
        
        updateDisplay();
        letzteAktionZeit = millis();
      } else {
        // SERIELLE AUSGABE: Info, dass die maximale Länge erreicht ist
        Serial.println("Maximale Sequenzlänge (5) erreicht. Eingabe ignoriert.");
      }
    }
    letzterTasterStatus = HIGH;
  }

  // 4. Logik zum Auswerten nach Timeout
  if (aktuelleMorseSequenz.length() > 0 && (millis() - letzteAktionZeit > TIMEOUT_MS)) {
    // SERIELLE AUSGABE: Start der Dekodierung
    Serial.println("\n--- Timeout ---");
    Serial.println("Dekodiere Sequenz: " + aktuelleMorseSequenz);
    
    char buchstabe = dekodiereMorse(aktuelleMorseSequenz);
    angezeigterText += buchstabe;
    
    // SERIELLE AUSGABE: Ergebnis der Dekodierung und der gesamte Text
    Serial.println("Ergebnis: '" + String(buchstabe) + "'");
    Serial.println("Gesamttext: " + angezeigterText);
    Serial.println("-----------------");

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

// Zeichnet das OLED-Display komplett neu
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 5);
  display.setTextWrap(true);
  display.print(angezeigterText);
  display.drawLine(0, 38, SCREEN_WIDTH, 38, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.setTextWrap(false);
  display.print(">");
  display.print(aktuelleMorseSequenz);
  display.display();
}