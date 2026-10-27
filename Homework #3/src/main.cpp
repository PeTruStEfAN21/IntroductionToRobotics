#include <Arduino.h>

// Definirea pinilor conform cerinței
const int PIN_BUILTIN_LED = 2;       // Heartbeat LED
const int PIN_EXTERNAL_LED = 18;     // LED-ul care simulează ventilatorul (PWM)
const int PIN_POTENTIOMETER = 34;    // Potențiometrul (intrare analogică)
const int PIN_RGB_R = 25;            // RGB LED - Roșu
const int PIN_RGB_G = 26;            // RGB LED - Verde
const int PIN_RGB_B = 27;            // RGB LED - Albastru
const int PIN_BUZZER = 23;           // Buzzer

// RGB cu catod comun: pinul comun la GND, culoarea se aprinde pe HIGH
// (pune true pentru anod comun: pinul comun la 3.3V, culoarea se aprinde pe LOW)
const bool RGB_COMMON_ANODE = false;

// Setări pentru PWM (pe ESP32 se folosește LEDC)
const int freq = 5000;
const int ledChannel = 1;            // NU canalul 0: tone() folosește implicit canalul 0
const int resolution = 8;            // Rezoluție de 8 biți (valori 0 - 255)

// Canale PWM pentru RGB (ca să putem amesteca culorile)
const int rgbChannelR = 2;
const int rgbChannelG = 3;
const int rgbChannelB = 4;

// Verdele e mult mai puternic decât roșul, așa că pentru galben îl reducem
const int YELLOW_GREEN_LEVEL = 60;   // 0 - 255

// Praguri ADC (0 - 4095 împărțit în 3 părți egale)
const int THRESHOLD_LOW_MID = 1365;
const int THRESHOLD_MID_HIGH = 2730;
const int HYSTERESIS = 80;           // Bonus: evită comutarea rapidă lângă praguri

// Bonus: netezirea citirilor (filtru exponențial)
const float SMOOTHING = 0.15;        // 0 < SMOOTHING <= 1 (mai mic = mai neted)
float smoothedAdc = 0;

// Variabile pentru Heartbeat (fără delay!)
unsigned long previousMillisBlink = 0;
const long intervalBlink = 500;      // Clipește la fiecare 500ms
bool ledState = LOW;

// Variabile pentru Serial Debugging la intervale regulate (fără delay!)
unsigned long previousMillisSerial = 0;
const long intervalSerial = 500;     // Printează la fiecare 500ms

// Variabilă pentru memorarea stării anterioare a pragului (pentru buzzer)
int lastZone = -1;                   // 0 = jos, 1 = mediu, 2 = sus

// Determină zona; pragurile se mută cu HYSTERESIS în funcție de zona curentă,
// astfel încât trebuie depășit clar pragul ca să se schimbe culoarea
int computeZone(int adc, int previousZone) {
  int lowMid = THRESHOLD_LOW_MID;
  int midHigh = THRESHOLD_MID_HIGH;

  if (previousZone == 0) {
    lowMid += HYSTERESIS;
  } else if (previousZone == 1) {
    lowMid -= HYSTERESIS;
    midHigh += HYSTERESIS;
  } else if (previousZone == 2) {
    midHigh -= HYSTERESIS;
  }

  if (adc < lowMid) return 0;
  if (adc < midHigh) return 1;
  return 2;
}

// Intensități 0 - 255 pentru fiecare culoare
void setRgb(int r, int g, int b) {
  ledcWrite(rgbChannelR, RGB_COMMON_ANODE ? 255 - r : r);
  ledcWrite(rgbChannelG, RGB_COMMON_ANODE ? 255 - g : g);
  ledcWrite(rgbChannelB, RGB_COMMON_ANODE ? 255 - b : b);
}

void setup() {
  // Inițializare Serial pentru debugging
  Serial.begin(115200);

  // Configurare pini digitali
  pinMode(PIN_BUILTIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  // Configurare PWM pentru External LED (fan speed)
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(PIN_EXTERNAL_LED, ledChannel);

  // Configurare PWM pentru RGB
  ledcSetup(rgbChannelR, freq, resolution);
  ledcSetup(rgbChannelG, freq, resolution);
  ledcSetup(rgbChannelB, freq, resolution);
  ledcAttachPin(PIN_RGB_R, rgbChannelR);
  ledcAttachPin(PIN_RGB_G, rgbChannelG);
  ledcAttachPin(PIN_RGB_B, rgbChannelB);

  // Pornim filtrul de la valoarea reală, nu de la 0
  smoothedAdc = analogRead(PIN_POTENTIOMETER);
}

void loop() {
  unsigned long currentMillis = millis();

  // ==========================================
  // 1. SYSTEM HEARTBEAT (fără delay)
  // ==========================================
  if (currentMillis - previousMillisBlink >= intervalBlink) {
    previousMillisBlink = currentMillis;
    ledState = !ledState;
    digitalWrite(PIN_BUILTIN_LED, ledState);
  }

  // ==========================================
  // 2. CITIRE POTENȚIOMETRU ȘI CONTROL VITEZĂ
  // ==========================================
  int rawAdc = analogRead(PIN_POTENTIOMETER); // Valoare între 0 și 4095
  smoothedAdc += SMOOTHING * (rawAdc - smoothedAdc);
  int adcValue = (int)(smoothedAdc + 0.5);

  // Conversie ADC (0 - 4095) în interval PWM (0 - 255)
  int pwmValue = map(adcValue, 0, 4095, 0, 255);
  ledcWrite(ledChannel, pwmValue); // Setează luminozitatea LED-ului extern

  // ==========================================
  // 3. RGB STATUS INDICATOR
  // ==========================================
  int currentZone = computeZone(adcValue, lastZone);
  const char *levelStr;

  if (currentZone == 0) {
    levelStr = "Low (Blue)";
    setRgb(0, 0, 255);
  }
  else if (currentZone == 1) {
    levelStr = "Medium (Yellow)";
    setRgb(255, YELLOW_GREEN_LEVEL, 0);
  }
  else {
    levelStr = "High (Red)";
    setRgb(255, 0, 0);
  }

  // ==========================================
  // 4. SOUND NOTIFICATIONS (Buzzer)
  // ==========================================
  // Se declanșează DOAR la intrarea în zona superioară
  // (tone() cu durată nu blochează pe ESP32)
  if (currentZone == 2 && lastZone != 2) {
    tone(PIN_BUZZER, 1000, 200); // Sunet scurt
  }
  lastZone = currentZone;

  // ==========================================
  // 6. SERIAL DEBUGGING (fără delay în buclă)
  // ==========================================
  if (currentMillis - previousMillisSerial >= intervalSerial) {
    previousMillisSerial = currentMillis;

    Serial.print("ADC Raw: ");
    Serial.print(rawAdc);
    Serial.print(" | ADC Smoothed: ");
    Serial.print(adcValue);
    Serial.print(" | PWM Output: ");
    Serial.print(pwmValue);
    Serial.print(" | Level: ");
    Serial.println(levelStr);
  }

  // Nu folosim delay() în loop: bucla rulează complet fluid.
}
