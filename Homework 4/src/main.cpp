#include <Arduino.h>

/*
const int button = 21;
const int led = 18;
int pushButtonCounter = 0;
int ButtonState = LOW;
int LedState = LOW;
int lastReading = LOW;



unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 30;

void setup() {
  Serial.begin(115200);
  pinMode(button, INPUT_PULLDOWN);
  pinMode(led, OUTPUT);
  digitalWrite(led, LedState);
}

void loop() {
  unsigned long currentTime = millis();
  int reading = digitalRead(button);

  if (reading != lastReading) {
    lastDebounceTime = currentTime;
  }

  if (currentTime - lastDebounceTime >= debounceDelay) {
    if (reading != ButtonState) {
      ButtonState = reading;

      if (ButtonState == HIGH) {
        LedState = !LedState;
        digitalWrite(led, LedState);
        pushButtonCounter++;
        Serial.println("Button pressed");
        Serial.println(pushButtonCounter);
      }
    }
  }

  lastReading = reading;
}
*/

/*
 * Homework #4 - Smart Lamp Controller
 *
 * Cablaj:
 *   - Buton: GPIO21 <-> GND (INPUT_PULLUP, FALLING) sau GPIO21 <-> 3V3
 *     (INPUT_PULLDOWN, RISING), ales prin BUTTON_ACTIVE_LOW
 *   - LED extern: GPIO18 -> rezistor 220-330 ohm -> anod LED, catod -> GND
 *   - Heartbeat: LED-ul de pe placa (GPIO2)
 *
 * Checkpoint-uri:
 *   1. Moduri OFF / DIM / FULL / BLINK, schimbate prin apasari debounced
 *   2. Blink non-blocant + heartbeat independent cu millis()
 *   3. Apasare scurta (mod urmator) vs. lunga (>= 1000 ms -> OFF imediat)
 *   4. ISR FALLING pe GPIO21 doar semnaleaza un posibil eveniment;
 *      validarea (debounce) si logica aplicatiei ruleaza in loop()
 *
 * Bonusuri:
 *   A. Dublu click (2 apasari scurte in 350 ms) -> FULL direct
 *   B. Moduri extra: BREATHE (respiratie) si SOS
 *   C. Statistici: intreruperi brute vs. apasari acceptate
 */

// ---------------- Pini si constante ----------------
const int button = 21;
const int led = 18;
const int onboardLed = 2;   // BUILTIN_LED e deja definit de framework-ul ESP32

// true  = buton intre GPIO21 si GND  (INPUT_PULLUP,   FALLING) - varianta ceruta in tema
// false = buton intre GPIO21 si 3V3  (INPUT_PULLDOWN, RISING)
const bool BUTTON_ACTIVE_LOW = true;
const int PRESSED  = BUTTON_ACTIVE_LOW ? LOW : HIGH;
const int RELEASED = BUTTON_ACTIVE_LOW ? HIGH : LOW;

const unsigned long debounceDelay = 30;       // nivel stabil cerut (ms)
const unsigned long longPressTime = 1000;     // prag apasare lunga (ms)
const unsigned long doubleClickTime = 350;    // fereastra dublu click (ms)
const unsigned long intervalBlink = 500;      // BLINK: 500 ms ON / 500 ms OFF
const unsigned long heartbeatInterval = 500;  // heartbeat LED placa
const unsigned long breathePeriod = 3000;     // o "respiratie" completa (ms)

const int DIM_LEVEL = 64;
const int FULL_LEVEL = 255;

// 1 = ciclul include modurile bonus (BLINK -> BREATHE -> SOS -> OFF)
// 0 = ciclul obligatoriu (BLINK -> OFF)
#define ENABLE_BONUS_MODES 1

// ---------------- ISR (Checkpoint 4 + Bonus C) ----------------
// ISR-ul doar noteaza ca s-a intamplat ceva. Nu decide nimic.
volatile bool buttonEventPending = false;
volatile unsigned long rawInterruptCount = 0;

void IRAM_ATTR onButtonISR() {
  buttonEventPending = true;
  rawInterruptCount = rawInterruptCount + 1;
}

// ---------------- Moduri lampa ----------------
enum LampMode {
  MODE_OFF,
  MODE_DIM,
  MODE_FULL,
  MODE_BLINK,
  MODE_BREATHE,
  MODE_SOS
};
LampMode lampMode = MODE_OFF;

unsigned long modeStartTime = 0;
bool blinkOn = false;
unsigned long lastBlinkToggle = 0;
int sosStep = 0;
unsigned long sosStepStart = 0;
int lastDuty = -1;

// SOS: ... --- ...   pasii pari = ON, impari = OFF (durate in ms)
const unsigned int sosPattern[] = {
  200, 200, 200, 200, 200, 600,    // S
  600, 200, 600, 200, 600, 600,    // O
  200, 200, 200, 200, 200, 1400    // S + pauza intre cuvinte
};
const int sosSteps = sizeof(sosPattern) / sizeof(sosPattern[0]);

const char* lampModeToString(LampMode mode) {
  switch (mode) {
    case MODE_OFF:     return "OFF";
    case MODE_DIM:     return "DIM";
    case MODE_FULL:    return "FULL";
    case MODE_BLINK:   return "BLINK";
    case MODE_BREATHE: return "BREATHE";
    case MODE_SOS:     return "SOS";
    default:           return "UNKNOWN";
  }
}

LampMode nextMode(LampMode mode) {
  switch (mode) {
    case MODE_OFF:     return MODE_DIM;
    case MODE_DIM:     return MODE_FULL;
    case MODE_FULL:    return MODE_BLINK;
#if ENABLE_BONUS_MODES
    case MODE_BLINK:   return MODE_BREATHE;
    case MODE_BREATHE: return MODE_SOS;
#endif
    default:           return MODE_OFF;
  }
}

void setLampMode(LampMode mode, unsigned long now) {
  lampMode = mode;
  modeStartTime = now;
  blinkOn = true;          // BLINK incepe cu LED-ul aprins
  lastBlinkToggle = now;
  sosStep = 0;
  sosStepStart = now;
  Serial.print("Mod lampa: ");
  Serial.println(lampModeToString(mode));
}

// ---------------- Actiuni aplicatie (separate de input) ----------------
bool singleClickPending = false;   // asteptam sa vedem daca vine al doilea click
unsigned long firstClickTime = 0;

unsigned long pressStartTime = 0;
bool longPressHandled = false;
unsigned long acceptedPressCount = 0;

void onShortPress(unsigned long now) {
  if (singleClickPending && now - firstClickTime <= doubleClickTime) {
    singleClickPending = false;
    Serial.println("Double click -> FULL");
    setLampMode(MODE_FULL, now);
  } else {
    // Nu executam inca: poate urmeaza al doilea click (Bonus A)
    singleClickPending = true;
    firstClickTime = now;
  }
}

void onLongPress(unsigned long now) {
  singleClickPending = false;
  Serial.println("Long press -> OFF");
  setLampMode(MODE_OFF, now);
}

void onConfirmedPress(unsigned long now) {
  pressStartTime = now;
  longPressHandled = false;
  acceptedPressCount++;

  // Bonus C: intreruperile brute sunt mai multe decat apasarile reale,
  // pentru ca un contact mecanic "sare" (bounce) si genereaza mai multe
  // fronturi FALLING la apasare si la eliberare; zgomotul electric poate
  // adauga si el fronturi false. Debounce-ul le filtreaza.
  unsigned long raw = rawInterruptCount;
  Serial.printf("Press confirmat | intreruperi brute: %lu | apasari acceptate: %lu\n",
                raw, acceptedPressCount);
}

void onConfirmedRelease(unsigned long now) {
  if (longPressHandled) {
    Serial.println("Release dupa long press (fara schimbare de mod)");
    return;
  }
  onShortPress(now);
}

// ---------------- Validare buton (debounce dupa intrerupere) ----------------
enum ButtonPhase {
  BTN_IDLE,              // asteptam notificarea din ISR
  BTN_VALIDATING_PRESS,  // confirmam LOW stabil
  BTN_PRESSED            // apasat; asteptam HIGH stabil (release)
};
ButtonPhase buttonPhase = BTN_IDLE;
int lastReading = RELEASED;
unsigned long lastChangeTime = 0;

void updateButton(unsigned long now) {
  if (buttonPhase == BTN_IDLE) {
    if (!buttonEventPending) return;   // fara intrerupere -> fara digitalRead
    buttonEventPending = false;
    buttonPhase = BTN_VALIDATING_PRESS;
    lastReading = digitalRead(button);
    lastChangeTime = now;
    return;
  }

  // Polling permis doar in timpul validarii
  int reading = digitalRead(button);
  if (reading != lastReading) {
    lastReading = reading;
    lastChangeTime = now;
  }
  bool stable = (now - lastChangeTime >= debounceDelay);

  switch (buttonPhase) {
    case BTN_VALIDATING_PRESS:
      if (stable) {
        if (reading == PRESSED) {
          buttonPhase = BTN_PRESSED;
          onConfirmedPress(now);
        } else {
          // HIGH stabil: a fost doar zgomot
          buttonPhase = BTN_IDLE;
          buttonEventPending = false;
          Serial.println("Intrerupere respinsa (zgomot)");
        }
      }
      break;

    case BTN_PRESSED:
      if (stable && reading == RELEASED) {
        buttonPhase = BTN_IDLE;
        buttonEventPending = false;    // ignoram fronturile din bounce-ul de release
        onConfirmedRelease(now);
      } else if (!longPressHandled && now - pressStartTime >= longPressTime) {
        longPressHandled = true;       // o singura data per apasare
        onLongPress(now);
      }
      break;

    default:
      break;
  }
}

// Click simplu executat abia dupa ce expira fereastra de dublu click
void updateClicks(unsigned long now) {
  if (singleClickPending && now - firstClickTime > doubleClickTime) {
    singleClickPending = false;
    Serial.println("Short press");
    setLampMode(nextMode(lampMode), now);
  }
}

// ---------------- Iesire LED extern (non-blocant) ----------------
void updateLamp(unsigned long now) {
  int duty = 0;

  switch (lampMode) {
    case MODE_OFF:
      duty = 0;
      break;

    case MODE_DIM:
      duty = DIM_LEVEL;
      break;

    case MODE_FULL:
      duty = FULL_LEVEL;
      break;

    case MODE_BLINK:
      if (now - lastBlinkToggle >= intervalBlink) {
        lastBlinkToggle += intervalBlink;
        blinkOn = !blinkOn;
      }
      duty = blinkOn ? FULL_LEVEL : 0;
      break;

    case MODE_BREATHE: {
      unsigned long phase = (now - modeStartTime) % breathePeriod;
      unsigned long half = breathePeriod / 2;
      unsigned long linear = (phase < half)
                               ? phase * 255 / half
                               : (breathePeriod - phase) * 255 / half;
      duty = linear * linear / 255;   // corectie gamma simpla, pare mai natural
      break;
    }

    case MODE_SOS:
      while (now - sosStepStart >= sosPattern[sosStep]) {
        sosStepStart += sosPattern[sosStep];
        sosStep = (sosStep + 1) % sosSteps;
      }
      duty = (sosStep % 2 == 0) ? FULL_LEVEL : 0;
      break;
  }

  if (duty != lastDuty) {   // scriem PWM doar la schimbare
    analogWrite(led, duty);
    lastDuty = duty;
  }
}

// ---------------- Heartbeat (independent de lampa) ----------------
bool heartbeatState = LOW;
unsigned long lastHeartbeat = 0;

void updateHeartbeat(unsigned long now) {
  if (now - lastHeartbeat >= heartbeatInterval) {
    lastHeartbeat += heartbeatInterval;
    heartbeatState = !heartbeatState;
    digitalWrite(onboardLed, heartbeatState);
  }
}

// ---------------- Setup / loop ----------------
void setup() {
  Serial.begin(115200);
  pinMode(button, BUTTON_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
  pinMode(led, OUTPUT);
  pinMode(onboardLed, OUTPUT);
  analogWrite(led, 0);
  lastDuty = 0;

  attachInterrupt(digitalPinToInterrupt(button), onButtonISR,
                  BUTTON_ACTIVE_LOW ? FALLING : RISING);

  unsigned long now = millis();
  lastHeartbeat = now;
  setLampMode(MODE_OFF, now);
}

void loop() {
  unsigned long now = millis();

  updateButton(now);
  updateClicks(now);
  updateLamp(now);
  updateHeartbeat(now);
}
