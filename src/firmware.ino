/*
 * Watch V1 Firmware
 * ESP32-C3 SuperMini + 5643A-1 4-digit 7-segment display (common cathode)
 * Two capacitive touch buttons: Mode (GPIO1), LED Control (GPIO3)
 * LDR ambient light sensor on GPIO6 (ADC1_CH6) with 10k pull-down to GND
 * RGB LED: common cathode on GPIO16 (R), GPIO17 (G), GPIO18 (B) — party mode via PWM
 * Haptic motor: GPIO15 via 2N2222 low-side switch, flyback diode to +3.3V
 * TP4056 charger: CHRG on GPIO4, STDBY on GPIO5 — charging status indication
 * Battery voltage monitor: ADC1_CH2 on GPIO2 via 100k/100k divider
 *
 * Features: time display, mode switching (HH:MM, MM:SS, MM:DD),
 *           RGB LED color control (short press = next color/OFF, long press = party mode),
 *           auto-brightness via LDR, deep-sleep after 3s inactivity,
 *           battery voltage monitoring, charging status indication,
 *           Wi-Fi + NTP time sync (optional, configure SSID/PASS below)
 *
 * Pin mapping matches KiCad schematic (assets/watch_v1/watch_v1.kicad_sch)
 * and ASCII diagram (src/ascii_schematic.txt). NO 74HC595 shift register —
 * segments driven directly from GPIO7-14 through 220Ω resistors.
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_sntp.h>
#include "esp_sleep.h"
#include "driver/adc.h"

void triggerHaptics(uint16_t durationMs = 50);
void setLedColor(uint8_t r, uint8_t g, uint8_t b);
void updatePartyLed();
void readBatteryVoltage();
void updateChargingStatus();

// ----- Wi-Fi Settings -----
// Replace with your network credentials or leave empty to skip NTP sync.
// const char* ssid     = "YOUR_SSID";
// const char* password = "YOUR_PASSWORD";

// ----- Pin Definitions (matching KiCad schematic & ASCII diagram) -----
// Segment lines — direct GPIO to 220Ω resistor to 5643A-1 segments (NO shift register)
const uint8_t SEG_PINS[8] = {7, 8, 9, 10, 11, 12, 13, 14};
// Digit enable pins (NPN 2N2222 bases via 4.7kΩ resistors)
const uint8_t digitPins[4] = {0, 2, 4, 5};
// Buttons (capacitive touch on ESP32-C3)
const uint8_t btnMode = 1;
const uint8_t btnLed  = 3;
// LDR (ambient light)
const uint8_t ldrPin  = 6;
// RGB LED — common cathode, 220Ω per channel
const uint8_t PIN_R = 16;
const uint8_t PIN_G = 17;
const uint8_t PIN_B = 18;
// Haptic motor
const uint8_t hapticsPin = 15;
// TP4056 status pins
const uint8_t chrgPin = 4;
const uint8_t stdbyPin = 5;
// Battery voltage monitor
const uint8_t batVpin = 2;

const uint16_t TOUCH_THRESHOLD = 400;

// ----- Segment bit order: a b c d e f g dp -----
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

const uint8_t COLON_ON  = 0b10000000;
const uint8_t COLON_OFF = 0b00000000;

// ----- State Variables -----
enum DisplayMode { MODE_HOUR_MIN, MODE_MIN_SEC, MODE_MONTH_DAY };
DisplayMode curMode = MODE_HOUR_MIN;

RTC_DATA_ATTR uint8_t hours   = 0;
RTC_DATA_ATTR uint8_t minutes = 0;
RTC_DATA_ATTR uint8_t seconds = 0;
RTC_DATA_ATTR uint8_t month   = 1;
RTC_DATA_ATTR uint8_t day     = 1;

const unsigned long debounceDelay = 50;
const unsigned long longPressTime = 500;
bool btnModePressed = false;
bool btnLedPressed  = false;
unsigned long btnModePressStart = 0;
unsigned long btnLedPressStart  = 0;

volatile int8_t encoderDelta = 0;
volatile bool   encoderChanged = false;

const unsigned int basePulseUs = 2000;
uint16_t ldrRaw = 0;
float brightnessFactor = 1.0;

// RGB LED party mode
enum LedMode { LED_OFF, LED_COLOR, LED_PARTY };
LedMode ledMode = LED_COLOR;
uint8_t currentColorIdx = 0;
const uint8_t numColors = 6;
const uint8_t presetColors[][3] = {
  {255,   0,   0},
  {0,   255,   0},
  {0,     0, 255},
  {255, 255,   0},
  {255,   0, 255},
  {0,   255, 255}
};
unsigned long lastPartyUpdate = 0;
const unsigned long partyInterval = 60;
float hue = 0.0;

RTC_DATA_ATTR unsigned long lastActivity = 0;
const unsigned long INACTIVITY_MS = 3000;

float batteryVoltage = 0.0;
bool isCharging = false;
bool chargeComplete = false;
unsigned long lastBatteryRead = 0;
const unsigned long BATTERY_READ_INTERVAL = 5000;

bool ntpSynced = false;

void setupDisplayPins() {
  for (int i = 0; i < 8; i++) pinMode(SEG_PINS[i], OUTPUT);
  for (int i = 0; i < 4; i++) pinMode(digitPins[i], OUTPUT);
  pinMode(btnMode, INPUT_PULLUP);
  pinMode(btnLed,  INPUT_PULLUP);
  pinMode(ldrPin,  INPUT);
  ledcSetup(0, 5000, 8);
  ledcSetup(1, 5000, 8);
  ledcSetup(2, 5000, 8);
  ledcAttachPin(PIN_R, 0);
  ledcAttachPin(PIN_G, 1);
  ledcAttachPin(PIN_B, 2);
  setLedColor(0, 0, 0);
  pinMode(hapticsPin, OUTPUT);
  digitalWrite(hapticsPin, LOW);
  pinMode(chrgPin, INPUT_PULLUP);
  pinMode(stdbyPin, INPUT_PULLUP);
  pinMode(batVpin, INPUT);
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11);
  for (int i = 0; i < 4; i++) digitalWrite(digitPins[i], LOW);
}

void setSegments(uint8_t seg) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(SEG_PINS[i], (seg >> i) & 0x01);
  }
}

void displayDigit(uint8_t digitIdx, uint8_t value, bool showColon = false) {
  for (int i = 0; i < 4; i++) digitalWrite(digitPins[i], LOW);
  uint8_t seg = digitSegments[value];
  if (showColon) seg |= COLON_ON;
  else seg &= ~COLON_ON;
  setSegments(seg);
  digitalWrite(digitPins[digitIdx], HIGH);
  unsigned int pulseUs = (unsigned int)(basePulseUs * brightnessFactor);
  if (pulseUs < 100) pulseUs = 100;
  delayMicroseconds(pulseUs);
  digitalWrite(digitPins[digitIdx], LOW);
}

void updateDisplay() {
  switch (curMode) {
    case MODE_HOUR_MIN:
      displayDigit(0, hours / 10, false);
      displayDigit(1, hours % 10, (seconds % 2) == 0);
      displayDigit(2, minutes / 10, false);
      displayDigit(3, minutes % 10, false);
      break;
    case MODE_MIN_SEC:
      displayDigit(0, minutes / 10, false);
      displayDigit(1, minutes % 10, (seconds % 2) == 0);
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

uint16_t readTouchPin(uint8_t pin) {
  return touchRead(pin);
}

void handleButtons() {
  unsigned long now = millis();
  uint16_t modeTouch = readTouchPin(btnMode);
  uint16_t ledTouch  = readTouchPin(btnLed);
  bool modeTouched = (modeTouch < TOUCH_THRESHOLD);
  bool ledTouched  = (ledTouch < TOUCH_THRESHOLD);

  if (modeTouched && !btnModePressed) {
    btnModePressed = true;
    btnModePressStart = now;
  } else if (!modeTouched && btnModePressed) {
    btnModePressed = false;
    unsigned long pressDuration = now - btnModePressStart;
    if (pressDuration < longPressTime) {
      curMode = (DisplayMode)((curMode + 1) % 3);
      noteActivity();
      triggerHaptics(30);
    } else {
      minutes = (minutes + 5) % 60;
      if (minutes < 5) hours = (hours + 1) % 24;
      noteActivity();
      triggerHaptics(80);
    }
    delay(150);
  }

  if (ledTouched && !btnLedPressed) {
    btnLedPressed = true;
    btnLedPressStart = now;
  } else if (!ledTouched && btnLedPressed) {
    btnLedPressed = false;
    unsigned long pressDuration = now - btnLedPressStart;
    if (pressDuration < longPressTime) {
      currentColorIdx = (currentColorIdx + 1) % (numColors + 1);
      if (currentColorIdx < numColors) {
        ledMode = LED_COLOR;
        setLedColor(presetColors[currentColorIdx][0],
                    presetColors[currentColorIdx][1],
                    presetColors[currentColorIdx][2]);
      } else {
        ledMode = LED_OFF;
        setLedColor(0, 0, 0);
      }
      noteActivity();
      triggerHaptics(30);
    } else {
      if (ledMode == LED_PARTY) {
        ledMode = LED_COLOR;
        setLedColor(presetColors[currentColorIdx][0],
                    presetColors[currentColorIdx][1],
                    presetColors[currentColorIdx][2]);
      } else {
        ledMode = LED_PARTY;
        hue = 0.0;
      }
      noteActivity();
      triggerHaptics(80);
    }
    delay(150);
  }
}

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

void updateBrightness() {
  ldrRaw = analogRead(ldrPin);
  float norm = (float)ldrRaw / 4095.0;
  brightnessFactor = 1.0 - norm;
  if (brightnessFactor < 0.15) brightnessFactor = 0.15;
  if (brightnessFactor > 1.0) brightnessFactor = 1.0;
}

void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  ledcWrite(0, r);
  ledcWrite(1, g);
  ledcWrite(2, b);
}

void hsvToRgb(float h, uint8_t& r, uint8_t& g, uint8_t& b) {
  float s = 1.0, v = 0.8;
  int i = (int)(h * 6);
  float f = h * 6 - i;
  float p = v * (1 - s);
  float q = v * (1 - f * s);
  float t = v * (1 - (1 - f) * s);
  float rr, gg, bb;
  switch (i % 6) {
    case 0: rr = v; gg = t; bb = p; break;
    case 1: rr = q; gg = v; bb = p; break;
    case 2: rr = p; gg = v; bb = t; break;
    case 3: rr = p; gg = q; bb = v; break;
    case 4: rr = t; gg = p; bb = v; break;
    case 5: rr = v; gg = p; bb = q; break;
  }
  r = (uint8_t)(rr * 255);
  g = (uint8_t)(gg * 255);
  b = (uint8_t)(bb * 255);
}

void updatePartyLed() {
  if (ledMode != LED_PARTY) return;
  unsigned long now = millis();
  if (now - lastPartyUpdate < partyInterval) return;
  lastPartyUpdate = now;
  hue += 0.015;
  if (hue > 1.0) hue = 0.0;
  uint8_t r, g, b;
  hsvToRgb(hue, r, g, b);
  setLedColor(r, g, b);
}

void triggerHaptics(uint16_t durationMs) {
  digitalWrite(hapticsPin, HIGH);
  delay(durationMs);
  digitalWrite(hapticsPin, LOW);
}

void readBatteryVoltage() {
  unsigned long now = millis();
  if (now - lastBatteryRead < BATTERY_READ_INTERVAL) return;
  lastBatteryRead = now;
  uint16_t raw = analogRead(batVpin);
  float adcVoltage = (raw / 4095.0) * 3.3;
  batteryVoltage = adcVoltage * 2.0;
}

void updateChargingStatus() {
  isCharging = (digitalRead(chrgPin) == LOW);
  chargeComplete = (digitalRead(stdbyPin) == LOW);
}

void IRAM_ATTR encISR() {
  static uint8_t lastCLK = digitalRead(10);
  uint8_t clk = digitalRead(10);
  uint8_t dt  = digitalRead(20);
  if (clk != lastCLK) {
    if (dt != clk) encoderDelta++;
    else           encoderDelta--;
    encoderChanged = true;
  }
  lastCLK = clk;
}

void setupEncoder() {
  pinMode(10, INPUT_PULLUP);
  pinMode(20, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(10), encISR, CHANGE);
}

void applyEncoder() {
  if (!encoderChanged) return;
  noInterrupts();
  int8_t delta = encoderDelta;
  encoderDelta = 0;
  encoderChanged = false;
  interrupts();
  switch (curMode) {
    case MODE_HOUR_MIN:
      if ((seconds % 2) == 0) {
        minutes = (minutes + delta + 60) % 60;
      } else {
        hours = (hours + delta + 24) % 24;
      }
      break;
    case MODE_MIN_SEC:
      seconds = (seconds + delta + 60) % 60;
      break;
    case MODE_MONTH_DAY:
      day = (day + delta - 1) % 31 + 1;
      if (day < 1) day = 31;
      break;
  }
  noteActivity();
}

void noteActivity() { lastActivity = millis(); }

void checkInactivity() {
  unsigned long now = millis();
  if (now - lastActivity > INACTIVITY_MS) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    esp_sleep_enable_timer_wakeup(1000000ULL);
    esp_deep_sleep_start();
  }
}

void setupTime() {
  WiFiManager wm;
  if (!wm.autoConnect("WatchV1-Setup")) {
    Serial.println("Failed to connect, restarting...");
    ESP.restart();
  }
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  const long gmtOffset_sec = 7 * 3600;
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
    ntpSynced = true;
    updateTimeFromNTP();
  }
}

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

void setupBLE() {
  BLEDevice::init("WatchV1");
}

void loopBLE() {
}

void setup() {
  Serial.begin(115200);
  setupDisplayPins();
  setupEncoder();
  lastTick = millis();
  lastActivity = millis();
  setupTime();
}

void loop() {
  handleButtons();
  applyEncoder();
  updateTime();
  if (ntpSynced) updateTimeFromNTP();
  updateBrightness();
  updateDisplay();
  updatePartyLed();
  readBatteryVoltage();
  updateChargingStatus();
  noteActivity();
  checkInactivity();
}
