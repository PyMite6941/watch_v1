/*
 * Watch V1 Firmware
 * ESP32-C3 SuperMini + 5643A-1 4-digit 7-segment display (common cathode)
 * Two buttons: Mode (GPIO1), LED Control (GPIO3)
 * LDR ambient light sensor on GPIO6 (ADC) with 10k pull-down to GND
 * Rotary encoder (KY-040): CLK GPIO19, DT GPIO20, SW GPIO21 (optional)
 * RGB LED: common cathode on GPIO16 (R), GPIO17 (G), GPIO18 (B)
 * Features: time display, mode switching (HH:MM, MM:SS, MM:DD), button/encoder increment,
 *           Bluetooth placeholder, auto‑brightness via LDR, light sleep after 3s inactivity wakeable by buttons,
 *           RGB LED color control (short press = next color/OFF, long press = party mode),
 *           Hold Mode button to fast‑adjust time.
 */
 
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "esp_sleep.h"

// ----- Pin Definitions -----
// Segment pins (a,b,c,d,e,f,g,dp)
const uint8_t segPins[8] = {7, 8, 9, 10, 11, 12, 13, 14};
// Digit enable pins (NPN transistor bases) – also used for PWM brightness
const uint8_t digitPins[4] = {0, 2, 4, 5};
// Buttons
const uint8_t btnMode = 1; // BTN1: mode cycling (strapping pin, use external pull‑up)
const uint8_t btnLed  = 3; // BTN2: LED control (strapping pin, use external pull‑up)
// LDR (ambient light)
const uint8_t ldrPin  = 6; // LDR (ambient light) on strapping pin, use external 10k pull‑down to GND
// Rotary encoder (KY-040)
const uint8_t ENC_CLK = 19;
const uint8_t ENC_DT  = 20;
const uint8_t ENC_SW  = 21;   // optional push‑button
// RGB LED (common cathode)
const uint8_t PIN_R = 16;
const uint8_t PIN_G = 17;
const uint8_t PIN_B = 18;

// ----- Display Mapping -----
// Segment bit order: a b c d e f g dp
const uint8_t digitSegments[10] = {
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111  // 9
};

// Colon dots: we'll use dp of digit1 and digit2 maybe
const uint8_t COLON_ON  = 0b10000000; // dp bit
const uint8_t COLON_OFF = 0b00000000;

// ----- State Variables -----
enum DisplayMode { MODE_HOUR_MIN, MODE_MIN_SEC, MODE_MONTH_DAY };
DisplayMode curMode = MODE_HOUR_MIN;

// Time variables (simplified)
uint8_t hours   = 0;
uint8_t minutes = 0;
uint8_t seconds = 0;
uint8_t month   = 1;
uint8_t day     = 1;

// Button debounce & timing
unsigned long lastBtnTime = 0;
const unsigned long debounceDelay = 50;
const unsigned long longPressTime = 500; // ms to consider a long press
bool btnModePressed = false;
bool btnLedPressed  = false;
unsigned long btnModePressStart = 0;
unsigned long btnLedPressStart  = 0;

// Encoder handling
volatile int8_t encoderDelta = 0;   // +1 or -1 per detent
volatile bool   encoderChanged = false;

// Brightness control (LDR)
const unsigned int basePulseUs = 2000; // base pulse width for full brightness (2ms)
uint16_t ldrRaw = 0;
float brightnessFactor = 1.0; // 0.0 - 1.0

// Inactivity timer for light sleep
unsigned long lastActivity = 0;
const unsigned long INACTIVITY_MS = 3000;   // 3 seconds

// RGB LED control
enum LedMode { LED_NORMAL, LED_PARTY };
LedMode ledMode = LED_NORMAL;
uint8_t currentColorIdx = 0; // index into preset colors
const uint8_t numColors = 5; // OFF, Red, Blue, Green, White
// Preset colors: R,G,B (0-255)
const uint8_t presetColors[][3] = {
  {0,   0,   0}, // OFF
  {255,   0,   0}, // Red
  {0,   255,   0}, // Green
  {0,     0, 255}, // Blue
  {255, 255, 255}  // White
};
unsigned long lastPartyUpdate = 0;
const unsigned long partyInterval = 50; // ms between hue steps in party mode
float hue = 0.0; // 0-1

// ----- Functions -----
void setupDisplayPins() {
  for (int i = 0; i < 8; i++) pinMode(segPins[i], OUTPUT);
  for (int i = 0; i < 4; i++) pinMode(digitPins[i], OUTPUT);
  pinMode(btnMode, INPUT_PULLUP);
  pinMode(btnLed,  INPUT_PULLUP);
  pinMode(ldrPin,  INPUT);
  // Encoder pins
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT,  INPUT_PULLUP);
  pinMode(ENC_SW,  INPUT_PULLUP);   // if using the push‑button
  // RGB LED pins (common cathode: LOW = on)
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  // Initially off
  digitalWrite(PIN_R, HIGH);
  digitalWrite(PIN_G, HIGH);
  digitalWrite(PIN_B, HIGH);
  // Initially disable all digits
  for (int i = 0; i < 4; i++) digitalWrite(digitPins[i], LOW); // NPN: LOW = off
}

void displayDigit(uint8_t digitIdx, uint8_t value, bool showColon = false) {
  // Turn off all digits first
  for (int i = 0; i < 4; i++) digitalWrite(digitPins[i], LOW);
  // Set segment pins
  uint8_t seg = digitSegments[value];
  if (showColon) seg |= COLON_ON;
  else seg &= ~COLON_ON;
  for (int i = 0; i < 8; i++) {
    digitalWrite(segPins[i], (seg >> i) & 0x01);
  }
  // Enable selected digit with brightness‑adjusted pulse width
  digitalWrite(digitPins[digitIdx], HIGH); // NPN: HIGH = conduct
  unsigned int pulseUs = (unsigned int)(basePulseUs * brightnessFactor);
  if (pulseUs < 100) pulseUs = 100; // clamp minimum to avoid flicker
  delayMicroseconds(pulseUs);
  // Disable after delay
  digitalWrite(digitPins[digitIdx], LOW);
}

void updateDisplay() {
  switch (curMode) {
    case MODE_HOUR_MIN:
      // digit0: hours tens, digit1: hours units, digit2: minutes tens, digit3: minutes units
      displayDigit(0, hours / 10, false);
      displayDigit(1, hours % 10, (seconds % 2)==0 ? true : false); // blink colon on seconds even
      displayDigit(2, minutes / 10, false);
      displayDigit(3, minutes % 10, false);
      break;
    case MODE_MIN_SEC:
      displayDigit(0, minutes / 10, false);
      displayDigit(1, minutes % 10, (seconds % 2)==0 ? true : false);
      displayDigit(2, seconds / 10, false);
      displayDigit(3, seconds % 10, false);
      break;
    case MODE_MONTH_DAY:
      displayDigit(0, month / 10, false);
      displayDigit(1, month % 10, false);
      displayDigit(2, day / 10, false);
      displayDigit(3, day % 10, false);
      break;
  }
}

void handleButtons() {
  unsigned long now = millis();
  // Read raw states (LOW = pressed because INPUT_PULLUP)
  bool modeRaw = (digitalRead(btnMode) == LOW);
  bool ledRaw  = (digitalRead(btnLed)  == LOW);

  // ----- Mode button -----
  if (modeRaw && !btnModePressed) {
    // just pressed
    btnModePressed = true;
    btnModePressStart = now;
  } else if (!modeRaw && btnModePressed) {
    // just released
    btnModePressed = false;
    unsigned long pressDuration = now - btnModePressStart;
    if (pressDuration < longPressTime) {
      // short press -> cycle display mode
      curMode = (DisplayMode)((curMode + 1) % 3);
      noteActivity();
    } else {
      // long press -> fast adjust time (add minutes quickly)
      minutes = (minutes + 5) % 60;
      if (minutes < 5) hours = (hours + 1) % 24; // approximate
      noteActivity();
    }
    // debounce after release
    delay(150);
  }

  // ----- LED button -----
  if (ledRaw && !btnLedPressed) {
    btnLedPressed = true;
    btnLedPressStart = now;
  } else if (!ledRaw && btnLedPressed) {
    btnLedPressed = false;
    unsigned long pressDuration = now - btnLedPressStart;
    if (pressDuration < longPressTime) {
      // short press -> next LED color (including OFF)
      currentColorIdx = (currentColorIdx + 1) % numColors;
      setLedColor(presetColors[currentColorIdx][0],
                  presetColors[currentColorIdx][1],
                  presetColors[currentColorIdx][2]);
      noteActivity();
    } else {
      // long press -> toggle party mode
      ledMode = (ledMode == LED_NORMAL) ? LED_PARTY : LED_NORMAL;
      if (ledMode == LED_NORMAL) {
        // restore last solid color
        setLedColor(presetColors[currentColorIdx][0],
                    presetColors[currentColorIdx][1],
                    presetColors[currentColorIdx][2]);
      }
      noteActivity();
    }
    delay(150);
  }

  // If button still held down, we could repeat action; optional.
}

// Simple time tick using millis()
unsigned long lastTick = 0;
void updateTime() {
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    seconds++;
    if (seconds >= 60) { seconds = 0; minutes++; }
    if (minutes >= 60) { minutes = 0; hours++; }
    if (hours >= 24)   hours = 0;
  }
}

// Read LDR and update brightness factor (call periodically)
void updateBrightness() {
  ldrRaw = analogRead(ldrPin); // 0‑4095
  // LDR voltage divider: Vout = Vcc * (Rldr/(Rldr+10k)). In dark, Rldr high => Vout high => reading high.
  // We want brighter display in light, dim in dark.
  float norm = (float)ldrRaw / 4095.0;
  brightnessFactor = 1.0 - norm; // 1.0 in light, 0.0 in dark
  // Clamp to usable range (0.2 - 1.0) to avoid turning off completely
  if (brightnessFactor < 0.2) brightnessFactor = 0.2;
  if (brightnessFactor > 1.0) brightnessFactor = 1.0;
}

// ----- Encoder ISR -----
void IRAM_ATTR encISR() {
  static uint8_t lastCLK = digitalRead(ENC_CLK);
  uint8_t clk = digitalRead(ENC_CLK);
  uint8_t dt  = digitalRead(ENC_DT);
  if (clk != lastCLK) {                     // edge on CLK
    if (dt != clk) encoderDelta++;         // clockwise
    else           encoderDelta--;         // counter‑clockwise
    encoderChanged = true;
  }
  lastCLK = clk;
}

void setupEncoder() {
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT,  INPUT_PULLUP);
  pinMode(ENC_SW,  INPUT_PULLUP);   // if you use the push‑button
  attachInterrupt(digitalPinToInterrupt(ENC_CLK), encISR, CHANGE);
}

/* ----- Call this from loop() to apply encoder movement ----- */
void applyEncoder() {
  if (!encoderChanged) return;
  noInterrupts();
  int8_t delta = encoderDelta;
  encoderDelta = 0;
  encoderChanged = false;
  interrupts();

  // Example: use encoder to adjust the currently selected field
  switch (curMode) {
    case MODE_HOUR_MIN:
      if ((seconds % 2) == 0) { // colon on -> adjusting minutes
        minutes = (minutes + delta + 60) % 60;
      } else { // colon off -> adjusting hours
        hours   = (hours   + delta + 24) % 24;
      }
      break;
    case MODE_MIN_SEC:
      seconds = (seconds + delta + 60) % 60;
      break;
    case MODE_MONTH_DAY:
      day = (day + delta - 1) % 31 + 1;   // keep 1‑31
      if (day < 1) day = 31;
      // month wrap handled later if needed
      break;
  }
  noteActivity();
}

/* ----- RGB LED helpers ----- */
void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  // common cathode: LOW turns on the LED
  analogWrite(PIN_R, 255 - r);
  analogWrite(PIN_G, 255 - g);
  analogWrite(PIN_B, 255 - b);
}

void updatePartyLed() {
  if (ledMode != LED_PARTY) return;
  unsigned long now = millis();
  if (now - lastPartyUpdate < partyInterval) return;
  lastPartyUpdate = now;
  hue += 0.01; // increment hue
  if (hue > 1.0) hue -= 1.0;
  // Convert HSV to RGB (V=1, S=1)
  float r, g, b;
  int i = (int)(hue * 6);
  float f = hue * 6 - i;
  float p = 0;
  float q = 1 - f;
  float t = f;
  switch (i % 6) {
    case 0: r = 1; g = t; b = 0; break;
    case 1: r = q; g = 1; b = 0; break;
    case 2: r = 0; g = 1; b = t; break;
    case 3: r = 0; g = q; b = 1; break;
    case 4: r = t; g = 0; b = 1; break;
    case 5: r = 1; g = 0; b = q; break;
  }
  setLedColor((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

/* ----- BLE placeholder (optional) ----- */
void setupBLE() {
  // Not implemented fully; just initialize BLE for future sync
  BLEDevice::init("WatchV1");
  // Could create server, advertise, etc.
}

void loopBLE() {
  // Placeholder
}

/* ----- Inactivity handling for light sleep ----- */
void noteActivity() { lastActivity = millis(); }

void checkInactivity() {
  unsigned long now = millis();
  if (now - lastActivity > INACTIVITY_MS) {
    // Enable wakeup on btnMode or btnLed (ext1) and go to light sleep
    uint64_t pinMask = (1ULL << btnMode) | (1ULL << btnLed);
    esp_sleep_enable_ext1_wakeup(pinMask, ESP_EXT1_WAKEUP_ANY_LOW);
    // Optional: also allow wakeup by timer (e.g., 30s) to prevent sleeping forever
    esp_sleep_enable_timer_wakeup(30 * 1000000ULL); // 30 seconds
    esp_light_sleep_start();
    // After waking, we need to re‑initialize peripherals that may have been powered down.
    // For simplicity, we just continue; the ESP32‑C3 retains GPIO state in light sleep.
  }
}

void setup() {
  Serial.begin(115200);
  setupDisplayPins();
  setupEncoder();
  // setupBLE();
  lastTick = millis();
  lastActivity = millis(); // start activity timer
  // Initialize LED to first color (OFF)
  setLedColor(presetColors[currentColorIdx][0],
              presetColors[currentColorIdx][1],
              presetColors[currentColorIdx][2]);
}

void loop() {
  handleButtons();
  applyEncoder();          // ← use encoder to change values
  updateTime();
  updateBrightness();      // adjust brightness based on ambient light
  updateDisplay();
  // loopBLE();
  updatePartyLed();        // handle party mode LED cycling
  noteActivity();          // any loop pass counts as activity (optional)
  checkInactivity();       // → light sleep after 3 s idle
}