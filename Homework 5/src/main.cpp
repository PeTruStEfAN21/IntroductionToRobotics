#include <Arduino.h>

/*
 * Homework #5 - Stop the Counter!
 *
 * Joc de reactie: display-ul arata o tinta, jucatorul porneste o numaratoare
 * inversa de la o valoare mai mare si trebuie sa o opreasca cat mai aproape
 * de tinta. Eroare = |valoare oprita - tinta|.
 *
 * Cablaj (acelasi ca in Laboratorul 4, alimentare la 3.3V):
 *   - 74HC595: DS -> GPIO23, SHCP -> GPIO18, STCP -> GPIO22
 *     Q7..Q0 -> segmentele A, B, C, D, E, F, G, DP (rezistor 1k pe fiecare)
 *   - Display 4 cifre: D1..D4 -> GPIO25, 26, 27, 33
 *   - Joystick: VRx -> GPIO34, VRy -> GPIO35, SW -> GPIO32 (INPUT_PULLUP)
 *
 * Controale:
 *   - Sus:            porneste numaratoarea
 *   - Buton (SW):     opreste numaratoarea
 *   - Jos:            reset (tinta noua)
 *   - Stanga/Dreapta: nivel de dificultate (inainte de start)
 *                     sau schimba ce se vede pe display (dupa oprire)
 *
 * Bonusuri: timp cu punct zecimal, tinte aleatoare, niveluri de dificultate,
 * cel mai bun scor, efecte pe display, Hacker Mode (Sus, Sus, Stanga, Dreapta
 * dupa oprire).
 */

// ---------------- Pini ----------------
const uint8_t dataPin = 23;   // DS
const uint8_t clockPin = 18;  // SHCP
const uint8_t latchPin = 22;  // STCP

const uint8_t displayCount = 4;
const uint8_t displayDigits[displayCount] = {25, 26, 27, 33};  // D1..D4

const uint8_t joystickXPin = 34;
const uint8_t joystickYPin = 35;
const uint8_t joystickSWPin = 32;

// ---------------- Tipul display-ului ----------------
// false = catod comun (cifra activa pe LOW, segment aprins pe HIGH)
const bool COMMON_ANODE = false;
const uint8_t DIGIT_ON = COMMON_ANODE ? HIGH : LOW;
const uint8_t DIGIT_OFF = COMMON_ANODE ? LOW : HIGH;

// ---------------- Masti de segmente ----------------
// Un octet per caracter: bit0 = A, bit1 = B, ... bit6 = G, bit7 = DP
const uint8_t SEG_A = 1 << 0;
const uint8_t SEG_B = 1 << 1;
const uint8_t SEG_C = 1 << 2;
const uint8_t SEG_D = 1 << 3;
const uint8_t SEG_E = 1 << 4;
const uint8_t SEG_F = 1 << 5;
const uint8_t SEG_G = 1 << 6;
const uint8_t SEG_DP = 1 << 7;
const uint8_t BLANK = 0;

const uint8_t displayEncodings[10] = {
  0b00111111,  // 0
  0b00000110,  // 1
  0b01011011,  // 2
  0b01001111,  // 3
  0b01100110,  // 4
  0b01101101,  // 5
  0b01111101,  // 6
  0b00000111,  // 7
  0b01111111,  // 8
  0b01101111   // 9
};

// Litere construite din bitii segmentelor
const uint8_t CH_A = SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
const uint8_t CH_b = SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t CH_E = SEG_A | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t CH_G = SEG_A | SEG_C | SEG_D | SEG_E | SEG_F;
const uint8_t CH_L = SEG_D | SEG_E | SEG_F;
const uint8_t CH_O = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F;
const uint8_t CH_P = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G;
const uint8_t CH_r = SEG_E | SEG_G;
const uint8_t CH_S = SEG_A | SEG_C | SEG_D | SEG_F | SEG_G;
const uint8_t CH_t = SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t CH_DASH = SEG_G;

const uint8_t TEXT_STOP[displayCount] = {CH_S, CH_t, CH_O, CH_P};
const uint8_t TEXT_ERR[displayCount] = {CH_E, CH_r, CH_r, BLANK};
const uint8_t TEXT_GOAL[displayCount] = {CH_G, CH_O, CH_A, CH_L};
const uint8_t TEXT_BEST[displayCount] = {CH_b, CH_E, CH_S, CH_t};
const uint8_t TEXT_NONE[displayCount] = {CH_DASH, CH_DASH, CH_DASH, CH_DASH};

// ---------------- Formatul numerelor ----------------
// true  = timp in sutimi de secunda, afisat S.cc (913 -> "9.13")
// false = intreg simplu (42 -> "42")
const bool TIME_FORMAT = true;
const uint8_t DP_DIGIT = 1;  // cifra dupa care sta punctul zecimal (0 = D1)

// ---------------- Setari joc ----------------
const uint8_t LEVEL_COUNT = 3;
// Cate unitati scade numaratoarea la fiecare 10 ms (nivel 1 = timp real)
const uint8_t levelSpeed[LEVEL_COUNT] = {1, 2, 3};

const uint32_t TRAVEL_MIN_MS = 4000;  // timp real intre start si tinta
const uint32_t TRAVEL_MAX_MS = 8000;

const uint32_t LABEL_MS = 700;         // cat sta un text pe display
const uint32_t RECORD_BLINK_MS = 1600; // clipire la record nou
const uint32_t BLINK_HALF_MS = 128;    // putere a lui 2, vezi blinkIsOff()

// ---------------- Setari intrari ----------------
const uint32_t DIGIT_PERIOD_US = 2000;  // 4 cifre x 2 ms = 125 Hz
const uint32_t JOYSTICK_INTERVAL_MS = 10;
const uint32_t DEBOUNCE_MS = 30;

// Masurat pe modulul meu: repaus ~2000, capatul de sus 4095, capatul de jos
// doar ~500-700, deci pragul trebuie sa fie sub 1200
const int JOY_PRESS_THRESHOLD = 900;    // peste: directie activa
const int JOY_RELEASE_THRESHOLD = 450;  // sub: inapoi in centru (histerezis)

// Orientarea modulului pe breadboard: VRx e pe verticala (sus = valoare mare),
// VRy pe orizontala (stanga = valoare mare)
const bool JOY_SWAP_AXES = true;
const bool JOY_INVERT_X = true;
const bool JOY_INVERT_Y = true;

// Centrul masurat la pornire e acceptat doar daca e aproape de mijlocul scalei
const int JOY_CENTER_DEFAULT = 2000;
const int JOY_CENTER_TOLERANCE = 500;

// ---------------- Stare joc ----------------
enum GameState {
  STATE_IDLE,     // se vede tinta, asteptam start
  STATE_RUNNING,  // numaratoarea merge
  STATE_RESULT,   // oprit, valoarea ramane pe display
  STATE_HACK      // editare secreta a rezultatului
};

enum ResultView { VIEW_STOP, VIEW_ERROR, VIEW_GOAL, VIEW_BEST, VIEW_COUNT };

enum Direction { DIR_NONE, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

GameState gameState = STATE_IDLE;
uint8_t resultView = VIEW_STOP;
uint8_t level = 0;  // 0..LEVEL_COUNT-1

uint16_t targetValue = 0;
uint16_t startValue = 0;
uint16_t stoppedValue = 0;
uint16_t errorValue = 0;
uint32_t countdownStartMs = 0;

uint16_t bestError[LEVEL_COUNT];
bool hasBest[LEVEL_COUNT] = {false, false, false};
uint32_t recordBlinkStartMs = 0;
bool recordBlinkActive = false;

// Text temporar (nume de ecran, nivel)
uint8_t labelBuffer[displayCount];
uint32_t labelStartMs = 0;
bool labelActive = false;

// Hacker Mode
const Direction secretSequence[] = {DIR_UP, DIR_UP, DIR_LEFT, DIR_RIGHT};
const uint8_t secretLength = sizeof(secretSequence) / sizeof(secretSequence[0]);
uint8_t secretProgress = 0;
uint8_t hackDigits[displayCount];
uint8_t hackSelected = 0;

// ---------------- Display ----------------
uint8_t displayBuffers[displayCount] = {BLANK, BLANK, BLANK, BLANK};
uint8_t blinkDigits = 0;    // bitul i = cifra i clipeste
uint8_t blinkKeepMask = 0;  // segmente care raman aprinse cand cifra e "stinsa"
uint8_t currentDigit = 0;
uint32_t lastRefreshUs = 0;

// Trimite cei 8 biti in registru, bitul 0 primul (ajunge pe Q7)
void writeSegments(uint8_t mask) {
  if (COMMON_ANODE) mask = ~mask;
  digitalWrite(latchPin, LOW);
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(dataPin, (mask >> i) & 1);
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);
  }
  digitalWrite(latchPin, HIGH);
}

bool blinkIsOff(uint32_t now) {
  return (now / BLINK_HALF_MS) & 1;
}

// Multiplexare non-blocanta: la fiecare 2 ms trecem la cifra urmatoare
void refreshDisplay(uint32_t now) {
  uint32_t nowUs = micros();
  if (nowUs - lastRefreshUs < DIGIT_PERIOD_US) return;
  lastRefreshUs = nowUs;

  digitalWrite(displayDigits[currentDigit], DIGIT_OFF);  // anti-ghosting
  currentDigit = (currentDigit + 1) & 0b11;              // 0..3

  uint8_t mask = displayBuffers[currentDigit];
  if (((blinkDigits >> currentDigit) & 1) && blinkIsOff(now)) {
    mask &= blinkKeepMask;
  }
  writeSegments(mask);
  digitalWrite(displayDigits[currentDigit], DIGIT_ON);
}

void showText(const uint8_t text[displayCount]) {
  for (uint8_t i = 0; i < displayCount; i++) {
    displayBuffers[i] = text[i];
  }
}

// Numar cu zerourile din fata stinse. In format de timp, cifra unitatilor
// de secunda si sutimile raman mereu vizibile (0.07, nu .07 si nu 00.07).
void showNumber(uint16_t number) {
  uint8_t firstForced = TIME_FORMAT ? DP_DIGIT : displayCount - 1;
  uint16_t divisor = 1000;
  bool leading = true;

  for (uint8_t i = 0; i < displayCount; i++) {
    uint8_t digit = (number / divisor) % 10;
    divisor /= 10;
    if (digit != 0 || i >= firstForced) leading = false;
    displayBuffers[i] = leading ? BLANK : displayEncodings[digit];
  }
  if (TIME_FORMAT) displayBuffers[DP_DIGIT] |= SEG_DP;  // doar bitul DP
}

void showLabel(const uint8_t text[displayCount], uint32_t now) {
  for (uint8_t i = 0; i < displayCount; i++) {
    labelBuffer[i] = text[i];
  }
  labelStartMs = now;
  labelActive = true;
}

// ---------------- Serial ----------------
void printValue(uint16_t value) {
  if (TIME_FORMAT) {
    Serial.printf("%u.%02u", value / 100, value % 100);
  } else {
    Serial.print(value);
  }
}

// ---------------- Logica jocului ----------------
uint16_t countdownValue(uint32_t now) {
  uint32_t dropped = (now - countdownStartMs) * levelSpeed[level] / 10;
  return dropped >= startValue ? 0 : startValue - dropped;
}

// Tinta aleatoare; valoarea de start e aleasa astfel incat pana la tinta
// sa treaca 4-8 secunde reale, indiferent de nivel
void newRound() {
  uint16_t speed = levelSpeed[level];
  uint32_t travelMs = random(TRAVEL_MIN_MS, TRAVEL_MAX_MS + 1);

  targetValue = random(100 * speed, 1000 * speed);
  startValue = targetValue + travelMs * speed / 10;
  gameState = STATE_IDLE;
  secretProgress = 0;
  recordBlinkActive = false;

  Serial.printf("\n--- Runda noua | Nivel %u (viteza x%u) ---\n", level + 1, speed);
  Serial.print("Target: ");
  printValue(targetValue);
  Serial.print(" | Start: ");
  printValue(startValue);
  Serial.println(" | Sus = start");
}

void startCountdown(uint32_t now) {
  countdownStartMs = now;
  gameState = STATE_RUNNING;
  Serial.println("Start! Buton = stop");
}

void finishRound(uint16_t value, uint32_t now, const char *reason) {
  stoppedValue = value;
  errorValue = stoppedValue > targetValue ? stoppedValue - targetValue
                                          : targetValue - stoppedValue;
  gameState = STATE_RESULT;
  resultView = VIEW_STOP;
  secretProgress = 0;

  bool newRecord = !hasBest[level] || errorValue < bestError[level];
  if (newRecord) {
    hasBest[level] = true;
    bestError[level] = errorValue;
    recordBlinkStartMs = now;
  }
  recordBlinkActive = newRecord;

  Serial.printf("[%s] Target: ", reason);
  printValue(targetValue);
  Serial.print(" | Stopped: ");
  printValue(stoppedValue);
  Serial.print(" | Error: ");
  printValue(errorValue);
  if (errorValue == 0) Serial.print(" | PERFECT!");
  if (newRecord) Serial.print(" | Record nou pe nivel!");
  Serial.println();
}

void changeLevel(int8_t step, uint32_t now) {
  level = (level + LEVEL_COUNT + step) % LEVEL_COUNT;
  newRound();

  const uint8_t text[displayCount] = {CH_L, BLANK, BLANK, displayEncodings[level + 1]};
  showLabel(text, now);
}

void changeView(int8_t step, uint32_t now) {
  resultView = (resultView + VIEW_COUNT + step) % VIEW_COUNT;
  recordBlinkActive = false;

  switch (resultView) {
    case VIEW_STOP:  showLabel(TEXT_STOP, now); break;
    case VIEW_ERROR: showLabel(TEXT_ERR, now);  break;
    case VIEW_GOAL:  showLabel(TEXT_GOAL, now); break;
    case VIEW_BEST:  showLabel(TEXT_BEST, now); break;
  }
}

// ---------------- Hacker Mode ----------------
// Intoarce true daca miscarea face parte din secventa secreta
bool advanceSecret(Direction dir) {
  if (dir != secretSequence[secretProgress]) {
    secretProgress = (dir == secretSequence[0]) ? 1 : 0;
    return false;
  }
  secretProgress++;
  return true;
}

void enterHackMode() {
  uint16_t value = stoppedValue;
  for (int8_t i = displayCount - 1; i >= 0; i--) {
    hackDigits[i] = value % 10;
    value /= 10;
  }
  hackSelected = displayCount - 1;
  secretProgress = 0;
  labelActive = false;
  gameState = STATE_HACK;
  Serial.println("[HACK] Mod editare: Stanga/Dreapta = cifra, Sus/Jos = valoare, Buton = confirma");
}

void handleHackInput(Direction dir, bool pressed, uint32_t now) {
  switch (dir) {
    case DIR_LEFT:  hackSelected = (hackSelected + displayCount - 1) & 0b11; break;
    case DIR_RIGHT: hackSelected = (hackSelected + 1) & 0b11; break;
    case DIR_UP:    hackDigits[hackSelected] = (hackDigits[hackSelected] + 1) % 10; break;
    case DIR_DOWN:  hackDigits[hackSelected] = (hackDigits[hackSelected] + 9) % 10; break;
    default: break;
  }

  if (pressed) {
    uint16_t value = 0;
    for (uint8_t i = 0; i < displayCount; i++) {
      value = value * 10 + hackDigits[i];
    }
    finishRound(value, now, "HACKED");
  }
}

// ---------------- Intrari ----------------
int joyCenterX = 2000;
int joyCenterY = 2000;
Direction joyDirection = DIR_NONE;
uint32_t lastJoystickReadMs = 0;

bool buttonReading = false;
bool buttonStable = false;
uint32_t buttonChangeMs = 0;

// Intoarce o directie o singura data, cand joystick-ul iese din centru.
// Pentru urmatoarea trebuie sa revina in centru (edge detection + histerezis).
Direction readJoystick(uint32_t now) {
  if (now - lastJoystickReadMs < JOYSTICK_INTERVAL_MS) return DIR_NONE;
  lastJoystickReadMs = now;

  int dx = analogRead(joystickXPin) - joyCenterX;
  int dy = analogRead(joystickYPin) - joyCenterY;
  if (JOY_SWAP_AXES) {
    int swap = dx;
    dx = dy;
    dy = swap;
  }
  if (JOY_INVERT_X) dx = -dx;
  if (JOY_INVERT_Y) dy = -dy;
  int absX = abs(dx);
  int absY = abs(dy);

  if (joyDirection != DIR_NONE) {
    if (absX < JOY_RELEASE_THRESHOLD && absY < JOY_RELEASE_THRESHOLD) {
      joyDirection = DIR_NONE;
    }
    return DIR_NONE;
  }

  if (absX < JOY_PRESS_THRESHOLD && absY < JOY_PRESS_THRESHOLD) return DIR_NONE;

  if (absX > absY) {
    joyDirection = dx < 0 ? DIR_LEFT : DIR_RIGHT;
  } else {
    joyDirection = dy < 0 ? DIR_UP : DIR_DOWN;
  }
  return joyDirection;
}

// Debounce: apasarea e acceptata dupa 30 ms de nivel stabil, dar pressMs
// primeste momentul primului contact, ca debounce-ul sa nu strice scorul
bool readButtonPress(uint32_t now, uint32_t &pressMs) {
  bool reading = digitalRead(joystickSWPin) == LOW;
  if (reading != buttonReading) {
    buttonReading = reading;
    buttonChangeMs = now;
  }

  if (now - buttonChangeMs >= DEBOUNCE_MS && reading != buttonStable) {
    buttonStable = reading;
    if (buttonStable) {
      pressMs = buttonChangeMs;
      return true;
    }
  }
  return false;
}

void handleInput(uint32_t now) {
  Direction dir = readJoystick(now);
  uint32_t pressMs = now;
  bool pressed = readButtonPress(now, pressMs);

  // Cand apesi pe joystick il si inclini putin: ignoram directiile cat timp
  // butonul e apasat, ca un stop sa nu devina din greseala reset
  if (buttonReading) dir = DIR_NONE;

  switch (gameState) {
    case STATE_IDLE:
      if (dir == DIR_UP) startCountdown(now);
      else if (dir == DIR_DOWN) newRound();
      else if (dir == DIR_LEFT) changeLevel(-1, now);
      else if (dir == DIR_RIGHT) changeLevel(1, now);
      break;

    case STATE_RUNNING:
      if (pressed) {
        // Buton apasat chiar inainte de start: nu socotim timp negativ
        if ((int32_t)(pressMs - countdownStartMs) < 0) pressMs = countdownStartMs;
        finishRound(countdownValue(pressMs), now, "STOP");
      } else if (dir == DIR_DOWN) {
        gameState = STATE_IDLE;
        Serial.println("Reset. Sus = start");
      } else if (countdownValue(now) == 0) {
        finishRound(0, now, "TIMEOUT");
      }
      break;

    case STATE_RESULT:
      if (dir == DIR_NONE) break;
      if (advanceSecret(dir)) {
        if (secretProgress == secretLength) enterHackMode();
      } else if (dir == DIR_DOWN) {
        newRound();
      } else if (dir == DIR_LEFT) {
        changeView(-1, now);
      } else if (dir == DIR_RIGHT) {
        changeView(1, now);
      }
      break;

    case STATE_HACK:
      handleHackInput(dir, pressed, now);
      break;
  }
}

// ---------------- Ce se vede pe display ----------------
void render(uint32_t now) {
  blinkDigits = 0;
  blinkKeepMask = 0;

  if (labelActive) {
    if (now - labelStartMs < LABEL_MS) {
      showText(labelBuffer);
      return;
    }
    labelActive = false;
  }

  switch (gameState) {
    case STATE_IDLE:
      showNumber(targetValue);
      break;

    case STATE_RUNNING:
      showNumber(countdownValue(now));
      break;

    case STATE_RESULT:
      if (resultView == VIEW_STOP) {
        showNumber(stoppedValue);
        if (recordBlinkActive) {
          if (now - recordBlinkStartMs < RECORD_BLINK_MS) blinkDigits = 0b1111;
          else recordBlinkActive = false;
        }
      } else if (resultView == VIEW_ERROR) {
        showNumber(errorValue);
      } else if (resultView == VIEW_GOAL) {
        showNumber(targetValue);
      } else if (hasBest[level]) {
        showNumber(bestError[level]);
      } else {
        showText(TEXT_NONE);
      }
      break;

    case STATE_HACK:
      // Semn vizibil: toate punctele zecimale aprinse + cifra selectata clipeste.
      // Cand clipeste se sting doar segmentele cifrei, punctul ramane.
      for (uint8_t i = 0; i < displayCount; i++) {
        displayBuffers[i] = displayEncodings[hackDigits[i]] | SEG_DP;
      }
      blinkDigits = 1 << hackSelected;
      blinkKeepMask = SEG_DP;
      break;
  }
}

// ---------------- Setup / loop ----------------
void setup() {
  Serial.begin(115200);

  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  for (uint8_t i = 0; i < displayCount; i++) {
    digitalWrite(displayDigits[i], DIGIT_OFF);
    pinMode(displayDigits[i], OUTPUT);
  }
  writeSegments(BLANK);

  pinMode(joystickSWPin, INPUT_PULLUP);
  analogReadResolution(12);

  // Centrul joystick-ului difera de la un modul la altul: il masuram la pornire
  const uint8_t samples = 16;
  uint32_t sumX = 0;
  uint32_t sumY = 0;
  for (uint8_t i = 0; i < samples; i++) {
    sumX += analogRead(joystickXPin);
    sumY += analogRead(joystickYPin);
  }
  joyCenterX = sumX / samples;
  joyCenterY = sumY / samples;
  // Daca joystick-ul era tinut inclinat la pornire, masuratoarea e gresita:
  // folosim centrul nominal
  if (abs(joyCenterX - JOY_CENTER_DEFAULT) > JOY_CENTER_TOLERANCE) joyCenterX = JOY_CENTER_DEFAULT;
  if (abs(joyCenterY - JOY_CENTER_DEFAULT) > JOY_CENTER_TOLERANCE) joyCenterY = JOY_CENTER_DEFAULT;

  randomSeed(esp_random());

  Serial.println("\nHomework #5 - Stop the Counter!");
  Serial.println("Sus = start | Buton = stop | Jos = reset | Stanga/Dreapta = nivel");
  newRound();
}

void loop() {
  uint32_t now = millis();

  handleInput(now);
  render(now);
  refreshDisplay(now);
}
