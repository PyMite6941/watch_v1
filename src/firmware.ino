/*
 * Watch V1 Firmware
 * ESP32-C3 SuperMini + 5643A-1 4-digit 7-segment display (common cathode)
 * Two touch buttons: Mode (GPIO1), LED Control (GPIO3)
 * LDR ambient light sensor on GPIO6 (ADC) with 10k pull-down to GND
 * Rotary encoder (KY-040): CLK GPIO19, DT GPIO20, SW GPIO21 (optional)
 * RGB LED: common cathode on GPIO16 (R), GPIO17 (G), GPIO18 (B)
 * Features: time display, mode switching (HH:MM, MM:SS, MM:DD), button/encoder increment,
 *           Bluetooth placeholder, auto-brightness via LDR, deep sleep after 3s inactivity wakeable by touch/timer,
 *           RGB LED color control (short press = next color/OFF, long press = party mode),
 *           Hold Mode button to fast-adjust time.
 *           Wi-Fi + NTP time sync (optional, configure SSID/PASS below)
 *           Deep-sleep with periodic wake-up and touch buttons for ultra-low power.
	 */
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_sntp.h>
#include "esp_sleep.h"
void triggerHaptics(uint16_t durationMs = 50);

// ----- Wi-Fi Settings -----
// Replace with your network credentials or leave empty to skip NTP sync.
// const char* ssid     = "YOUR_SSID";     // <-- set your Wi-Fi SSID
// const char* password = "YOUR_PASSWORD"; // <-- set your Wi-Fi password

// ----- Pin Definitions -----
// 74HC595 shift register for segment lines
const uint8_t SR_DATA = 0;   // DS
const uint8_t SR_CLK  = 1;   // SH_CP
const uint8_t SR_LATCH= 2;   // ST_CP
// Wiring assumption: QA -> segment a, QB -> b, QC -> c, QD -> d, QE -> e, QF -> f, QG -> g, QH -> dp (colon)
// Use MSBFIRST so that bit0 (LSB) appears at QA, bit7 (MSB) at QH.
// Digit enable pins (NPN transistor bases)
const uint8_t digitPins[4] = {3, 4, 5, 6};
// Buttons
const uint8_t btnMode = 7; // BTN1: mode cycling
const uint8_t btnLed  = 8; // BTN2: LED control
// LDR (ambient light)
const uint8_t ldrPin  = 9; // LDR on ADC
// Rotary encoder (KY-040) – optional
const uint8_t ENC_CLK = 10;
const uint8_t ENC_DT  = 20;
const uint8_t ENC_SW  = 21;   // optional push-button
// RGB LED and haptic motor omitted (use I2C expander if needed)
// const uint8_t PIN_R = 16;
// const uint8_t PIN_G = 17;
// const uint8_t PIN_B = 18;
// const uint8_t hapticsPin = 15;

const uint16_t TOUCH_THRESHOLD = 400; // touch threshold for button detection

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

// Time variables (simplified) - retained across deep sleep
RTC_DATA_ATTR uint8_t hours   = 0;
RTC_DATA_ATTR uint8_t minutes = 0;
RTC_DATA_ATTR uint8_t seconds = 0;
RTC_DATA_ATTR uint8_t month   = 1;
RTC_DATA_ATTR uint8_t day     = 1;

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
RTC_DATA_ATTR unsigned long lastActivity = 0;
const unsigned long INACTIVITY_MS = 3000;   // 3 seconds

// RGB LED control - disabled
// enum LedMode { LED_NORMAL, LED_PARTY };
// LedMode ledMode = LED_NORMAL;
// uint8_t currentColorIdx = 0; // index into preset colors
// const uint8_t numColors = 5; // OFF, Red, Blue, Green, White
// // Preset colors: R,G,B (0-255)
// const uint8_t presetColors[][3] = {
//   {0,   0,   0}, // OFF
//   {255,   0,   0}, // Red
//   {0,   255,   0}, // Green
//   {0,     0, 255}, // Blue
//   {255, 255, 255}  // White
// };
// unsigned long lastPartyUpdate = 0;
// const unsigned long partyInterval = 50; // ms between hue steps in party mode
// float hue = 0.0; // 0-1

// NTP sync flag
bool ntpSynced = false;

// ----- Functions -----
void setupDisplayPins() {
  pinMode(SR_DATA, OUTPUT);
  pinMode(SR_CLK,  OUTPUT);
  pinMode(SR_LATCH,OUTPUT);
  for (int i = 0; i < 4; i++) pinMode(digitPins[i], OUTPUT);
  pinMode(btnMode, INPUT_PULLUP);
  pinMode(btnLed,  INPUT_PULLUP);
  pinMode(ldrPin,  INPUT);
  // Encoder pins
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT,  INPUT_PULLUP);
  pinMode(ENC_SW,  INPUT_PULLUP);   // if using the push-button
  // RGB LED and haptic motor not used; keep pins as inputs to avoid floating
  // pinMode(PIN_R, INPUT);
  // pinMode(PIN_G, INPUT);
  // pinMode(PIN_B, INPUT);
  // pinMode(hapticsPin, INPUT);
  // Initially disable all digits
  for (int i = 0; i < 4; i++) digitalWrite(digitPins[i], LOW); // NPN: LOW = off
}

void shiftOutSegments(uint8_t seg) {
  digitalWrite(SR_LATCH, LOW);
  shiftOut(SR_DATA, SR_CLK, MSBFIRST, seg);
  digitalWrite(SR_LATCH, HIGH);
}

void displayDigit(uint8_t digitIdx, uint8_t value, bool showColon = false) {
  // Turn off all digits first
  for (int i = 0; i < 4; i++) digitalWrite(digitPins[i], LOW);
  // Set segment pattern via shift register
  uint8_t seg = digitSegments[value];
  if (showColon) seg |= COLON_ON;
  else seg &= ~COLON_ON;
  shiftOutSegments(seg);
  // Enable selected digit with brightness-adjusted pulse width
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
  // Read touch values; lower means touched
  uint16_t modeTouch = digitalRead(btnMode) ? 4095 : 0;
  uint16_t ledTouch  = digitalRead(btnLed)  ? 4095 : 0;
  bool modeTouched = (modeTouch < TOUCH_THRESHOLD);
  bool ledTouched  = (ledTouch  < TOUCH_THRESHOLD);

  // ----- Mode button -----
  if (modeTouched && !btnModePressed) {
    // just touched
    btnModePressed = true;
    btnModePressStart = now;
  } else if (!modeTouched && btnModePressed) {
    // just released
    btnModePressed = false;
    unsigned long pressDuration = now - btnModePressStart;
    if (pressDuration < longPressTime) {
      // short press -> cycle display mode
      curMode = (DisplayMode)((curMode + 1) % 3);
      // triggerHaptics();
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
  if (ledTouched && !btnLedPressed) {
    btnLedPressed = true;
    btnLedPressStart = now;
  } else if (!ledTouched && btnLedPressed) {
    btnLedPressed = false;
    unsigned long pressDuration = now - btnLedPressStart;
    if (pressDuration < longPressTime) {
      // short press -> next LED color (including OFF)
      // currentColorIdx = (currentColorIdx + 1) % numColors;
      // setLedColor(presetColors[currentColorIdx][0],
      //             presetColors[currentColorIdx][1],
      //             presetColors[currentColorIdx][2]);
      noteActivity();
      // triggerHaptics();
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
  ldrRaw = analogRead(ldrPin); // 0-4095
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
    else           encoderDelta--;         // counter-clockwise
    encoderChanged = true;
  }
  lastCLK = clk;
}

void setupEncoder() {
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT,  INPUT_PULLUP);
  pinMode(ENC_SW,  INPUT_PULLUP);   // if you use the push-button
  attachInterrupt(digitalPinToInterrupt(ENC_CLK), encISR, CHANGE);
}

void triggerHaptics(uint16_t durationMs) {
  // Haptic motor disabled
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
      day = (day + delta - 1) % 31 + 1;   // keep 1-31
      if (day < 1) day = 31;
      // month wrap handled later if needed
      break;
  }
  noteActivity();
    // triggerHaptics();
}

void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  // RGB LED disabled
}

void updatePartyLed() {
  // Party LED disabled
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
    // Power down Wi-Fi and BLE if active
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    // Enter deep sleep for 1 second
    esp_sleep_enable_timer_wakeup(1000000ULL); // 1 second
    esp_deep_sleep_start();
  }
}

/* ----- Wi-Fi + NTP time sync ----- */
void setupTime() {
  // WiFiManager
  WiFiManager wm;
  // Optional: set AP password
  // wm.setAPPassword("config123");
  // Try to autoConnect; if fails, start AP
  if (!wm.autoConnect("WatchV1-Setup")) {
    Serial.println("Failed to connect, restarting...");
    ESP.restart();
  }
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // NTP sync
  const long gmtOffset_sec = 7 * 3600;   // Thailand ICT
  const int daylightOffset_sec = 0;
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP sync");
  unsigned long waitStart = millis();
  while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && millis() - waitStart < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
    Serial.println("\nNTP sync failed – continuing with local clock");
  } else {
    Serial.println("\nNTP synced");
    updateTimeFromNTP(); // seed RTC-retained time vars
  }
}

/* Pull current time from NTP (if synced) and update globals */
void updateTimeFromNTP() {
  if (!ntpSynced) return;
  time_t now;
  struct tm timeinfo;
  if (time(&now) == 0) return;
  if (localtime_r(&now, &timeinfo) == NULL) return;
  hours   = timeinfo.tm_hour;
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;
  month   = timeinfo.tm_mon + 1;
  day     = timeinfo.tm_mday;
}

/* ----- Arduino setup & loop ----- */
void setup() {
  Serial.begin(115200);
  setupDisplayPins();
  pinMode(btnMode, INPUT_PULLUP);
  pinMode(btnLed, INPUT_PULLUP);
  setupEncoder();
  // setupBLE();   // uncomment if you want BLE initialized
  lastTick = millis();
  lastActivity = millis(); // start activity timer
  // Initialize LED to first color (OFF)
  // setLedColor(presetColors[currentColorIdx][0],
  //              presetColors[currentColorIdx][1],
  //              presetColors[currentColorIdx][2]);
  // Attempt Wi-Fi + NTP sync
  setupTime();
}

void loop() {
  handleButtons();
  applyEncoder();          // ← use encoder to change values
  updateTime();            // local millis() based tick
  updateTimeFromNTP();     // override with NTP if available
  updateBrightness();      // adjust brightness based on ambient light
  updateDisplay();
  // loopBLE();             // uncomment if BLE used
  // updatePartyLed();        // handle party mode LED cycling
  noteActivity();          // any loop pass counts as activity (optional)
  checkInactivity();       // → light sleep after 3 s idle
}
