/*
 * Watch V1 firmware - ESP32-C3 SuperMini
 *
 * Files
 *   config.h      pins and factory defaults
 *   types.h       shared types, the Settings struct
 *   settings.h    settings <-> JSON <-> flash
 *   timesource.h  Wi-Fi + where the time comes from (NTP or an HTTP API)
 *   weather.h     weather from the project's web service
 *   mesh.h        watch-to-watch messages (ESP-NOW)
 *   env.h         environment sensor (BME280) and the sample log
 *   tuner.h       instrument tuner (microphone on the accessory socket)
 *   apps.h        files on the watch, and custom apps made from /apps/*.json
 *   link.h        configuration commands over USB and Bluetooth
 *   watch_v1.ino  this file: display, buttons, screens, sleep
 *
 * Display : 4-digit common-cathode 7-segment on two daisy-chained 74HC595
 *           shift registers (U2 = segments, U3 = digits + RGB LED + haptic).
 * Power   : deep sleep almost all the time. A button wakes the watch, it shows
 *           a screen for a few seconds and sleeps again. It also wakes by
 *           itself for bells, a finished timer, a log sample or a time sync.
 *
 * Buttons : MODE short   next screen
 *           MODE long    set the clock by hand
 *           SET          depends on the screen - see docs/USER_GUIDE.md
 *           both, 2 s    link mode: Bluetooth on for Watch Studio
 *
 * Build   : Arduino core "esp32" 3.x, board "ESP32C3 Dev Module",
 *           USB CDC On Boot = Enabled, Partition Scheme = "Minimal SPIFFS".
 *           Library: ArduinoJson 7. See docs/BUILD_GUIDE.md.
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "driver/usb_serial_jtag.h"
#include "config.h"
#include "types.h"
#include "settings.h"
#include "timesource.h"
#include "weather.h"
#include "mesh.h"
#include "env.h"
#include "tuner.h"
#include "apps.h"
#include "link.h"

// ------------------------------------------------------------------ glyphs
// bit0 = a, bit1 = b, ... bit6 = g, bit7 = dp
static const uint8_t DIGITS[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};
#define SEG_DP    0x80
#define SEG_BLANK 0x00
#define SEG_DASH  0x40

// Best-effort seven-segment alphabet (some letters only exist in one case).
static uint8_t glyph(char c) {
  if (c >= '0' && c <= '9') return DIGITS[c - '0'];
  switch (c) {
    case 'A': case 'a': return 0x77;
    case 'B': case 'b': return 0x7C;
    case 'C': return 0x39;
    case 'c': return 0x58;
    case 'D': case 'd': return 0x5E;
    case 'E': case 'e': return 0x79;
    case 'F': case 'f': return 0x71;
    case 'G': case 'g': return 0x3D;
    case 'H': return 0x76;
    case 'h': return 0x74;
    case 'I': return 0x06;
    case 'i': return 0x04;
    case 'J': case 'j': return 0x1E;
    case 'L': case 'l': return 0x38;
    case 'N': case 'n': return 0x54;
    case 'O': return 0x3F;
    case 'o': return 0x5C;
    case 'P': case 'p': return 0x73;
    case 'Q': case 'q': return 0x67;
    case 'R': case 'r': return 0x50;
    case 'S': case 's': return 0x6D;
    case 'T': case 't': return 0x78;
    case 'U': return 0x3E;
    case 'u': return 0x1C;
    case 'Y': case 'y': return 0x6E;
    case '-': return SEG_DASH;
    case '_': return 0x08;
    case '*': return 0x63;      // degree sign
    default:  return SEG_BLANK;
  }
}

// ------------------------------------------------------- state kept in sleep
RTC_DATA_ATTR time_t          lastSyncEpoch  = 0;
RTC_DATA_ATTR static uint8_t  savedView      = 0;     // index into settings.views
RTC_DATA_ATTR static bool     swRunning      = false; // stopwatch
RTC_DATA_ATTR static int64_t  swStartMs      = 0;
RTC_DATA_ATTR static int64_t  swHeldMs       = 0;
RTC_DATA_ATTR static int64_t  timerEndMs     = 0;     // countdown; 0 = not running
RTC_DATA_ATTR static uint16_t tally          = 0;     // counter
RTC_DATA_ATTR static uint8_t  lastRoll       = 0;     // dice
RTC_DATA_ATTR static uint32_t lastBellMinute = 0;     // so a bell rings once
RTC_DATA_ATTR static uint32_t lastWarnMinute = 0;

// --------------------------------------------------------------- display mux
static volatile uint8_t frame[4]  = {0, 0, 0, 0};  // segment pattern per digit
static volatile uint8_t auxBits   = 0;             // RGB + haptic bits for U3
static uint8_t          muxDigit  = 0;
static esp_timer_handle_t muxTimer = nullptr;
static portMUX_TYPE     srLock    = portMUX_INITIALIZER_UNLOCKED;

static inline void shiftOut16(uint16_t value) {
  for (int8_t i = 15; i >= 0; i--) {
    digitalWrite(PIN_SR_DATA, (value >> i) & 1);
    digitalWrite(PIN_SR_CLK, HIGH);
    digitalWrite(PIN_SR_CLK, LOW);
  }
  digitalWrite(PIN_SR_LATCH, HIGH);
  digitalWrite(PIN_SR_LATCH, LOW);
}

// Runs every MUX_PERIOD_US: blank, then light the next digit.
static void muxTick(void *) {
  uint8_t aux = auxBits;
  muxDigit = (muxDigit + 1) & 3;
  uint16_t word = ((uint16_t)(aux | (1 << muxDigit)) << 8) | frame[muxDigit];
  portENTER_CRITICAL(&srLock);
  shiftOut16((uint16_t)aux << 8);   // all digits off first: no ghosting
  shiftOut16(word);
  portEXIT_CRITICAL(&srLock);
}

static void setBrightness(uint8_t level) {   // 0-255
  // /OE is active low, so invert: duty 0 = always on.
  ledcWrite(PIN_SR_OE, 255 - level);
}

static void displayBegin() {
  pinMode(PIN_SR_OE, OUTPUT);
  digitalWrite(PIN_SR_OE, HIGH);            // outputs off while we clear
  pinMode(PIN_SR_DATA, OUTPUT);
  pinMode(PIN_SR_CLK, OUTPUT);
  pinMode(PIN_SR_LATCH, OUTPUT);
  digitalWrite(PIN_SR_CLK, LOW);
  digitalWrite(PIN_SR_LATCH, LOW);
  shiftOut16(0);
  ledcAttach(PIN_SR_OE, 20000, 8);
  setBrightness(128);
  esp_timer_create_args_t args = {};
  args.callback = &muxTick;
  args.name = "mux";
  esp_timer_create(&args, &muxTimer);
  esp_timer_start_periodic(muxTimer, MUX_PERIOD_US);
}

static void displayEnd() {
  if (muxTimer) esp_timer_stop(muxTimer);
  ledcDetach(PIN_SR_OE);
  pinMode(PIN_SR_OE, OUTPUT);
  digitalWrite(PIN_SR_OE, HIGH);
  shiftOut16(0);                            // latch everything off for sleep
}

static void show(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  frame[0] = a; frame[1] = b; frame[2] = c; frame[3] = d;
}

static void showNumberPair(uint8_t left, uint8_t right, bool colon,
                           bool blankLeft = false, bool blankRight = false) {
  uint8_t s[4] = {
    DIGITS[(left / 10) % 10], DIGITS[left % 10], DIGITS[(right / 10) % 10], DIGITS[right % 10]
  };
  if (blankLeft)  { s[0] = SEG_BLANK; s[1] = SEG_BLANK; }
  if (blankRight) { s[2] = SEG_BLANK; s[3] = SEG_BLANK; }
  if (colon) s[COLON_DIGIT] |= SEG_DP;
  show(s[0], s[1], s[2], s[3]);
}

// Right-aligned number 0-9999, optional letter in the first digit.
static void showNumber(int value, char prefix = 0) {
  uint8_t s[4] = { SEG_BLANK, SEG_BLANK, SEG_BLANK, DIGITS[value % 10] };
  if (value >= 10)   s[2] = DIGITS[(value / 10) % 10];
  if (value >= 100)  s[1] = DIGITS[(value / 100) % 10];
  if (value >= 1000) s[0] = DIGITS[(value / 1000) % 10];
  if (prefix && value < 1000) s[0] = glyph(prefix);
  show(s[0], s[1], s[2], s[3]);
}

static void showString(const char *text) {
  uint8_t s[4] = { SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK };
  for (uint8_t i = 0; i < 4 && text[i]; i++) s[i] = glyph(text[i]);
  show(s[0], s[1], s[2], s[3]);
}

// A message that covers whatever screen is showing, for a moment.
static char     overlay[5] = "";
static uint32_t overlayUntil = 0;

void showText(const char *text, uint16_t ms) {
  strlcpy(overlay, text, sizeof(overlay));
  overlayUntil = millis() + ms;
  showString(overlay);
}

// Seconds as MM:SS, or HH:MM once it no longer fits.
static void showDuration(int64_t seconds, bool colon) {
  if (seconds < 0) seconds = 0;
  if (seconds < 6000) showNumberPair(seconds / 60, seconds % 60, colon);
  else showNumberPair((seconds / 3600) % 100, (seconds / 60) % 60, colon);
}

// -------------------------------------------------------------------- DS3231
#define DS3231_ADDR 0x68
bool rtcPresent = false;

static uint8_t bcd2bin(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
static uint8_t bin2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

// Days since 1970-01-01 for a UTC calendar date (Howard Hinnant's algorithm).
static int32_t daysFromCivil(int y, int m, int d) {
  y -= m <= 2;
  int era = (y >= 0 ? y : y - 399) / 400;
  int yoe = y - era * 400;
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

static bool rtcDetect() {
  Wire.beginTransmission(DS3231_ADDR);
  return Wire.endTransmission() == 0;
}

// Reads the DS3231 (kept in UTC). Returns false if its oscillator has stopped,
// which means the backup cell died and the time is garbage.
static bool rtcRead(time_t &out) {
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(0x0F);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom(DS3231_ADDR, 1) != 1) return false;
  if (Wire.read() & 0x80) return false;     // OSF flag

  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom(DS3231_ADDR, 7) != 7) return false;
  uint8_t ss = bcd2bin(Wire.read() & 0x7F);
  uint8_t mm = bcd2bin(Wire.read() & 0x7F);
  uint8_t hh = bcd2bin(Wire.read() & 0x3F);
  Wire.read();                              // day of week, unused
  uint8_t d  = bcd2bin(Wire.read() & 0x3F);
  uint8_t mo = bcd2bin(Wire.read() & 0x1F);
  int     y  = 2000 + bcd2bin(Wire.read());
  if (y < 2025 || mo < 1 || mo > 12 || d < 1 || d > 31) return false;
  out = (time_t)daysFromCivil(y, mo, d) * 86400 + hh * 3600UL + mm * 60UL + ss;
  return true;
}

static void rtcWrite(time_t t) {
  struct tm u;
  gmtime_r(&t, &u);
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(0x00);
  Wire.write(bin2bcd(u.tm_sec));
  Wire.write(bin2bcd(u.tm_min));
  Wire.write(bin2bcd(u.tm_hour));           // bit6 = 0 -> 24 h mode
  Wire.write(u.tm_wday + 1);
  Wire.write(bin2bcd(u.tm_mday));
  Wire.write(bin2bcd(u.tm_mon + 1));
  Wire.write(bin2bcd(u.tm_year % 100));
  Wire.endTransmission();
  // Clear the oscillator-stop flag, disable the 32 kHz output to save power.
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(0x0F);
  Wire.write(0x00);
  Wire.endTransmission();
}

// ---------------------------------------------------------------- time logic
bool timeValid() { return time(nullptr) > 1735689600; }  // after 2025-01-01

static void setSystemTime(time_t t) {
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
  settimeofday(&tv, nullptr);
}

// Sets the system clock and the DS3231. A running stopwatch or timer is
// shifted by the same amount, so correcting the clock does not disturb it.
void setClock(time_t utc) {
  int64_t shift = ((int64_t)utc - (int64_t)time(nullptr)) * 1000;
  if (swRunning) swStartMs += shift;
  if (timerEndMs) timerEndMs += shift;
  if (seqApp >= 0) seqEndMs += shift;
  if (logNextEpoch) logNextEpoch += shift / 1000;
  setSystemTime(utc);
  if (rtcPresent) rtcWrite(utc);
}

static void localNow(struct tm &out) {
  time_t t = time(nullptr);
  localtime_r(&t, &out);
}

// ------------------------------------------------------------------ sensors
uint16_t batteryMilliVolts() {
  uint32_t sum = 0;                          // long average: audio from the accessory
  for (uint8_t i = 0; i < 32; i++) sum += analogReadMilliVolts(PIN_VBAT_SENSE);   // port rides on this pin
  return (uint16_t)((sum / 32) * VBAT_DIVIDER);
}

uint8_t batteryPercent(uint16_t mv) {
  if (mv <= VBAT_EMPTY_MV) return 0;
  if (mv >= VBAT_FULL_MV) return 100;
  return (uint8_t)(((uint32_t)(mv - VBAT_EMPTY_MV) * 100) / (VBAT_FULL_MV - VBAT_EMPTY_MV));
}

bool isCharging() { return digitalRead(PIN_CHRG) == LOW; }

// True while a computer (a USB host) is talking to the USB-C port.
bool usbConnected() { return usb_serial_jtag_is_connected(); }

static int lightMilliVolts() { return analogReadMilliVolts(PIN_LDR_SENSE); }

static uint8_t ambientBrightness() {
  if (!settings.auto_bright) return settings.bright_max;
  long level = map(constrain(lightMilliVolts(), LDR_DARK_MV, LDR_BRIGHT_MV),
                   LDR_DARK_MV, LDR_BRIGHT_MV, settings.bright_min, settings.bright_max);
  return (uint8_t)level;
}

// ------------------------------------------------------------ haptic and LED
static uint32_t hapticUntil = 0;
static uint8_t  patLeft = 0;                 // buzz pattern still to play
static uint16_t patOn = 0, patOff = 0;
static uint32_t patNext = 0;
static uint8_t  ledColour   = 0;             // 0 = off, 1..7 = colour index
static bool     partyMode   = false;
static uint32_t ledSince    = 0;
static uint8_t  ledOverride = 0xFF;          // a screen (tuner) borrowing the LED; 0xFF = no
static const uint8_t COLOURS[8] = {
  0, AUX_LED_R, AUX_LED_G, AUX_LED_B,
  AUX_LED_R | AUX_LED_G, AUX_LED_G | AUX_LED_B, AUX_LED_R | AUX_LED_B,
  AUX_LED_R | AUX_LED_G | AUX_LED_B
};

void buzz(uint16_t ms) {
  hapticUntil = millis() + ms;
  auxBits = auxBits | AUX_HAPTIC;
}

void buzzPattern(uint8_t count, uint16_t onMs, uint16_t offMs) {
  patLeft = count;
  patOn = onMs;
  patOff = offMs;
  patNext = millis();
}

static void applyLed() {
  uint8_t bits = COLOURS[ledColour];
  if (partyMode) bits = COLOURS[1 + (millis() / PARTY_STEP_MS) % 7];   // a new colour twice a second
  if (ledOverride != 0xFF) bits = ledOverride;
  auxBits = (auxBits & AUX_HAPTIC) | bits;
}

static bool ledActive() { return ledColour != 0 || partyMode; }

// ------------------------------------------------------------------ buttons
static Button btnMode = { PIN_BTN_MODE, false, false, false, 0, 0 };
static Button btnSet  = { PIN_BTN_SET,  false, false, false, 0, 0 };

static void buttonBegin(Button &b) {
  pinMode(b.pin, INPUT_PULLUP);
  b.down = digitalRead(b.pin) == LOW;
  b.ignore = b.down;
  b.downAt = b.changedAt = millis();
}

static Event buttonPoll(Button &b) {
  uint32_t now = millis();
  bool raw = digitalRead(b.pin) == LOW;
  if (raw != b.down && now - b.changedAt >= DEBOUNCE_MS) {
    b.down = raw;
    b.changedAt = now;
    if (raw) {
      b.downAt = now;
      b.longFired = false;
    } else {
      bool wasIgnored = b.ignore, wasLong = b.longFired;
      b.ignore = false;
      if (!wasIgnored && !wasLong) return EV_SHORT;
    }
  }
  if (b.down && !b.ignore && !b.longFired && now - b.downAt >= LONG_PRESS_MS) {
    b.longFired = true;
    return EV_LONG;
  }
  return EV_NONE;
}

// ------------------------------------------------------------------- screens
static uint8_t  viewIndex = 0;               // index into settings.views
static bool     setMode = false;
static uint8_t  field = F_HOUR;
static struct tm editTm;
static uint32_t lastActivity = 0;
static uint32_t lastRepeat = 0;
static bool     linkMode = false;
static bool     metroOn = false;
static uint32_t metroNext = 0;
static EnvReading envNow = {};
static uint32_t envReadAt = 0;
static Pitch    pitchNow = {};
static uint32_t pitchAt = 0;
static uint32_t rollUntil = 0;
static uint32_t msgStart = 0;                // custom "message" app: when the scroll began
static uint8_t  wxPage = 0;                  // weather screen: which number is showing
enum { WX_TEMP, WX_SKY, WX_HIGH, WX_LOW, WX_RAIN, WX_SUNRISE, WX_SUNSET, WX_AIR, WX_PAGES };
static const char *const WX_LABELS[WX_PAGES] = { nullptr, nullptr, nullptr, nullptr, "rAIn", "rISE", "SEt ", "AQI " };

void keepAwake() { lastActivity = millis(); }

// MODE steps through the built-in screens chosen in settings, then the custom apps.
static uint8_t totalViews() { return settings.views_n + appCount; }
static int8_t  appIndex() { return viewIndex >= settings.views_n ? (int8_t)(viewIndex - settings.views_n) : -1; }
static uint8_t view() {
  if (viewIndex >= totalViews()) viewIndex = 0;
  return viewIndex < settings.views_n ? settings.views[viewIndex] : (uint8_t)VIEW_APP;
}

// Shows the loaded "message" app's text sliding across the four digits.
static bool renderMessage(uint32_t ms) {
  int len = strlen(app.text);
  int step = (ms - msgStart) / 320;
  if (step > len + 4) { showString(app.title); return false; }
  char win[5] = "    ";
  for (int i = 0; i < 4; i++) {
    int at = step - 4 + i;
    if (at >= 0 && at < len) win[i] = app.text[at];
  }
  showString(win);
  return true;
}

static void renderApp(uint32_t ms, bool blink) {
  int8_t idx = appIndex();
  appLoad(idx);
  switch (app.type) {
    case APP_SEQUENCE:
      if (seqApp == idx) showDuration((seqEndMs - nowMs() + 999) / 1000, blink);
      else showString(app.title);
      break;
    case APP_PICKER:
      if (ms < rollUntil) showNumber(1 + esp_random() % 99);                 // tumbling
      else if (pickApp == idx && pickLast >= 0) {
        if (app.n && pickLast < app.n && app.items[pickLast].label[0]) showString(app.items[pickLast].label);
        else showNumber(pickLast + 1);
      } else showString(app.title);
      break;
    case APP_COUNTDOWN: {
      long left = (long)(app.date - time(nullptr));
      if (!timeValid()) showString("----");
      else if (left <= 0) showString("donE");
      else if (left >= 86400) showNumber((left + 86399) / 86400, 'd');
      else showNumberPair(left / 3600, (left / 60) % 60, blink);
      break;
    }
    case APP_MESSAGE:
      if (renderMessage(ms)) keepAwake();
      break;
    default: showString(app.title);
  }
}

// Whole degrees, converted if the owner switched units since the last fetch.
static int wxDegrees(int16_t v) {
  if (wx.imperial == settings.fahrenheit) return v;
  return settings.fahrenheit ? (int)lroundf(v * 9.0f / 5 + 32) : (int)lroundf((v - 32) * 5.0f / 9);
}

static void renderWeather() {
  if (!wxConfigured()) { showString(" LOC"); return; }     // no place set: Watch Studio > Weather
  if (!wxFresh()) { showString("----"); return; }          // hold SET to fetch
  char buf[8];
  char unit = settings.fahrenheit ? 'F' : 'C';
  switch (wxPage) {
    case WX_TEMP: {
      int t = wxDegrees(wx.temp);
      if (t >= -9 && t <= 99) snprintf(buf, sizeof(buf), "%2d*%c", t, unit);
      else snprintf(buf, sizeof(buf), "%3d%c", t, unit);
      showString(buf);
      break;
    }
    case WX_SKY: showString(wxWord(wx.code)); break;
    case WX_HIGH: case WX_LOW: {
      int16_t v = wxPage == WX_HIGH ? wx.high : wx.low;
      if (v == WX_NONE) { showString("----"); break; }
      snprintf(buf, sizeof(buf), "%c%3d", wxPage == WX_HIGH ? 'H' : 'L', wxDegrees(v));
      showString(buf);
      break;
    }
    case WX_RAIN: showNumber(wx.rain > 100 ? 100 : wx.rain, 'r'); break;
    case WX_SUNRISE: case WX_SUNSET: {
      int16_t m = wxPage == WX_SUNRISE ? wx.sunrise : wx.sunset;
      if (m == WX_NONE) { showString("----"); break; }
      uint8_t h = m / 60;
      if (!settings.h24) h = h % 12 ? h % 12 : 12;
      showNumberPair(h, m % 60, true);
      if (!settings.h24 && h < 10) frame[0] = SEG_BLANK;
      break;
    }
    case WX_AIR:
      if (wx.aqi == WX_NONE || wx.aqi < 0) showString("----");
      else showNumber(wx.aqi > 9999 ? 9999 : wx.aqi);
      break;
  }
}

static int64_t stopwatchMs() { return swHeldMs + (swRunning ? nowMs() - swStartMs : 0); }

// Seconds until the next bell on an enabled day, or -1. With warn = true the
// early-warning times count as well (used to decide when to wake up).
static long secondsToNextBell(bool warn) {
  if (!timeValid() || settings.alerts_n == 0) return -1;
  struct tm t;
  localNow(t);
  long secsToday = t.tm_hour * 3600L + t.tm_min * 60L + t.tm_sec;
  long best = -1;
  for (int day = 0; day <= 7; day++) {
    if (!(settings.alert_days & (1 << ((t.tm_wday + day) % 7)))) continue;
    for (uint8_t i = 0; i < settings.alerts_n; i++) {
      for (int pass = 0; pass < (warn && settings.alert_warn_min ? 2 : 1); pass++) {
        long minutes = (long)settings.alerts[i] - (pass ? settings.alert_warn_min : 0);
        long delta = day * 86400L + minutes * 60L - secsToday;
        if (delta > 0 && (best < 0 || delta < best)) best = delta;
      }
    }
    if (best >= 0) break;
  }
  return best;
}

// Rings the bell / early warning when the clock reaches one. Safe to call often.
static bool bellCheck() {
  if (!timeValid() || settings.alerts_n == 0) return false;
  struct tm t;
  localNow(t);
  if (!(settings.alert_days & (1 << t.tm_wday))) return false;
  uint32_t minuteStamp = (uint32_t)(time(nullptr) / 60);
  int minutes = t.tm_hour * 60 + t.tm_min;
  for (uint8_t i = 0; i < settings.alerts_n; i++) {
    if (minutes == settings.alerts[i] && lastBellMinute != minuteStamp) {
      lastBellMinute = minuteStamp;
      buzzPattern(3, 250, 150);
      showText("bELL", 3000);
      keepAwake();
      return true;
    }
    if (settings.alert_warn_min && minutes == (int)settings.alerts[i] - settings.alert_warn_min &&
        lastWarnMinute != minuteStamp) {
      lastWarnMinute = minuteStamp;
      buzzPattern(1, 400, 0);
      char msg[5];
      snprintf(msg, sizeof(msg), "%2u_ ", settings.alert_warn_min);   // "5_ " = five minutes left
      showText(msg, 3000);
      keepAwake();
      return true;
    }
  }
  return false;
}

static bool timerCheck() {
  if (!timerEndMs || nowMs() < timerEndMs) return false;
  timerEndMs = 0;
  buzzPattern(4, 200, 120);
  showText("End ", 3000);
  keepAwake();
  return true;
}

static void logSample() {
  EnvReading r = envRead();
  logAdd(r, lightMilliVolts());
  logNextEpoch = time(nullptr) + (time_t)settings.log_min * 60;
}

static void renderView() {
  struct tm t;
  localNow(t);
  uint32_t ms = millis();
  bool blink = (ms / 500) & 1;
  uint8_t v = view();
  bool needsClock = v == VIEW_TIME || v == VIEW_SECONDS || v == VIEW_DATE || v == VIEW_PERIOD;
  if (needsClock && !timeValid()) {
    show(SEG_DASH, SEG_DASH | (blink ? SEG_DP : 0), SEG_DASH, SEG_DASH);
    return;
  }
  ledOverride = 0xFF;
  switch (v) {
    case VIEW_APP: renderApp(ms, blink); break;
    case VIEW_TIME: {
      uint8_t hour = settings.h24 ? t.tm_hour : (t.tm_hour % 12 ? t.tm_hour % 12 : 12);
      showNumberPair(hour, t.tm_min, blink, false, false);
      if (!settings.h24 && hour < 10) frame[0] = SEG_BLANK;
      if (!settings.h24 && t.tm_hour >= 12) frame[3] = frame[3] | SEG_DP;   // last dot = PM
      break;
    }
    case VIEW_SECONDS: showNumberPair(t.tm_min, t.tm_sec, true);  break;
    case VIEW_DATE:    showNumberPair(t.tm_mon + 1, t.tm_mday, true); break;
    case VIEW_BATTERY: {
      uint8_t pct = batteryPercent(batteryMilliVolts());
      showNumber(pct, 'b');
      if (isCharging() && blink) frame[3] = frame[3] | SEG_DP;   // dot blinks while charging
      break;
    }
    case VIEW_STOPWATCH: showDuration(stopwatchMs() / 1000, !swRunning || blink); break;
    case VIEW_TIMER:
      if (timerEndMs) showDuration((timerEndMs - nowMs() + 999) / 1000, blink);
      else showDuration(settings.timer_s, true);
      break;
    case VIEW_COUNTER: showNumber(tally); break;
    case VIEW_DICE:
      if (ms < rollUntil) showNumber(1 + esp_random() % settings.dice, 'd');   // tumbling
      else if (lastRoll) showNumber(lastRoll, 'd');
      else showString("dICE");
      break;
    case VIEW_PERIOD: {
      long left = secondsToNextBell(false);
      if (left < 0) showString("----"); else showDuration(left, blink);
      break;
    }
    case VIEW_MESH:
      if (!settings.mesh) showString(" OFF");
      else showNumber(meshPeerCount(), 'P');
      break;
    case VIEW_TEMP: case VIEW_HUMIDITY: case VIEW_PRESSURE: {
      if (ms - envReadAt > 2000 || envReadAt == 0) {
        envNow = envRead();
        envReadAt = ms ? ms : 1;
      }
      if (!envNow.ok || (v == VIEW_HUMIDITY && !envNow.hasHumidity)) { showString("----"); break; }
      if (v == VIEW_TEMP) {
        float temp = settings.fahrenheit ? envNow.tempC * 9 / 5 + 32 : envNow.tempC;
        int tenths = (int)lroundf(fabsf(temp) * 10);
        if (tenths > 999) tenths = 999;
        show(temp < 0 && tenths < 100 ? SEG_DASH : DIGITS[(tenths / 100) % 10], DIGITS[(tenths / 10) % 10] | SEG_DP,
             DIGITS[tenths % 10], glyph(settings.fahrenheit ? 'F' : 'C'));
        if (temp >= 0 && tenths < 100) frame[0] = SEG_BLANK;
      } else if (v == VIEW_HUMIDITY) {
        showNumber((int)lroundf(envNow.humidity), 'H');
      } else {
        showNumber((int)lroundf(envNow.hPa));
      }
      break;
    }
    case VIEW_TUNER: {
      // Listening blocks for ~150 ms, so do it a few times a second, not every pass.
      if (ms - pitchAt > 250) {
        Pitch p = tunerListen();
        if (p.ok) { pitchNow = p; keepAwake(); }
        else if (ms - pitchAt > 1500) pitchNow.ok = false;
        if (p.ok || !pitchNow.ok) pitchAt = millis();
      }
      if (!pitchNow.ok) { showString("tunE"); break; }
      static const char NAMES[12] = { 'C', 'C', 'd', 'd', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'b' };
      static const bool SHARP[12] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };
      int cents = abs(pitchNow.cents);
      show(glyph(NAMES[pitchNow.note]) | (SHARP[pitchNow.note] ? SEG_DP : 0),
           DIGITS[constrain(pitchNow.octave, 0, 9)], DIGITS[cents / 10], DIGITS[cents % 10]);
      // LED: green = in tune, red = flat (tighten), blue = sharp (loosen)
      ledOverride = cents <= 5 ? AUX_LED_G : (pitchNow.cents < 0 ? AUX_LED_R : AUX_LED_B);
      break;
    }
    case VIEW_METRONOME:
      showNumber(settings.bpm, metroOn ? 0 : 'b');
      break;
    case VIEW_WEATHER: renderWeather(); break;
  }
}

static void renderSet() {
  bool hide = ((millis() / 250) & 1) && !btnSet.down;   // edited field blinks
  switch (field) {
    case F_HOUR:   showNumberPair(editTm.tm_hour, editTm.tm_min, true, hide, false); break;
    case F_MINUTE: showNumberPair(editTm.tm_hour, editTm.tm_min, true, false, hide); break;
    case F_MONTH:  showNumberPair(editTm.tm_mon + 1, editTm.tm_mday, true, hide, false); break;
    case F_DAY:    showNumberPair(editTm.tm_mon + 1, editTm.tm_mday, true, false, hide); break;
    case F_YEAR: {
      int y = editTm.tm_year + 1900;
      if (hide) show(SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK);
      else show(DIGITS[y / 1000], DIGITS[(y / 100) % 10], DIGITS[(y / 10) % 10], DIGITS[y % 10]);
      break;
    }
  }
}

static uint8_t daysInMonth(int year, int month0) {
  static const uint8_t dm[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
  return month0 == 1 && leap ? 29 : dm[month0];
}

static void bumpField() {
  int year = editTm.tm_year + 1900;
  switch (field) {
    case F_HOUR:   editTm.tm_hour = (editTm.tm_hour + 1) % 24; break;
    case F_MINUTE: editTm.tm_min = (editTm.tm_min + 1) % 60; editTm.tm_sec = 0; break;
    case F_MONTH:  editTm.tm_mon = (editTm.tm_mon + 1) % 12; break;
    case F_DAY:    editTm.tm_mday = editTm.tm_mday % daysInMonth(year, editTm.tm_mon) + 1; break;
    case F_YEAR:   editTm.tm_year = (year >= 2040 ? 2025 : year + 1) - 1900; break;
  }
  uint8_t maxDay = daysInMonth(editTm.tm_year + 1900, editTm.tm_mon);
  if (editTm.tm_mday > maxDay) editTm.tm_mday = maxDay;
}

static void enterSetMode() {
  localNow(editTm);
  if (!timeValid()) {                      // sensible starting point
    editTm = {};
    editTm.tm_year = 2026 - 1900;
    editTm.tm_mday = 1;
  }
  field = F_HOUR;
  setMode = true;
}

static void saveSetMode() {
  editTm.tm_isdst = -1;
  setClock(mktime(&editTm));               // local time -> UTC epoch
  setMode = false;
  showText(" SEt", 600);
}

static void doSync() {
  meshStop();
  showString("SYnC");
  overlayUntil = 0;
  bool ok = timeSync();
  showText(ok ? "  On" : " Err", 800);
  buzz(ok ? 40 : 200);
  keepAwake();
}

// What SET does on each screen. Clock screens: the RGB LED.
static void onSet(Event e) {
  uint32_t now = millis();
  static const uint16_t TIMER_PRESETS[] = { 60, 120, 180, 300, 600, 900, 1200, 1500, 1800, 2700, 3600 };
  static const uint16_t BPM_PRESETS[] = { 60, 72, 80, 90, 100, 120, 140, 160 };
  switch (view()) {
    case VIEW_APP: {
      int8_t idx = appIndex();
      appLoad(idx);
      if (app.type == APP_SEQUENCE) {
        if (e == EV_LONG) seqStop();
        else if (seqApp == idx) seqEndMs = nowMs();          // skip to the next step
        else seqStart(idx);
      } else if (app.type == APP_PICKER) {
        if (!pickNext(idx)) buzzPattern(2, 80, 80);          // list used up: everyone has had a turn
        rollUntil = now + 500;
      } else if (app.type == APP_MESSAGE) {
        msgStart = now;
      }
      break;
    }
    case VIEW_STOPWATCH:
      if (e == EV_SHORT) {
        if (swRunning) { swHeldMs += nowMs() - swStartMs; swRunning = false; }
        else { swStartMs = nowMs(); swRunning = true; }
      } else { swRunning = false; swHeldMs = 0; }
      break;
    case VIEW_TIMER:
      if (e == EV_SHORT) timerEndMs = timerEndMs ? 0 : nowMs() + (int64_t)settings.timer_s * 1000;
      else if (!timerEndMs) {                               // long press: next preset length
        uint8_t i = 0;
        while (i < 11 && TIMER_PRESETS[i] <= settings.timer_s) i++;
        settings.timer_s = TIMER_PRESETS[i % 11];
        settingsSave();
      }
      break;
    case VIEW_COUNTER:
      if (e == EV_SHORT) tally = (tally + 1) % 10000; else tally = 0;
      break;
    case VIEW_DICE:
      lastRoll = 1 + esp_random() % settings.dice;
      rollUntil = now + 500;
      break;
    case VIEW_MESH:
      if (meshSend(MESH_BUZZ)) showText("SEnt", 700);
      break;
    case VIEW_METRONOME:
      if (e == EV_SHORT) { metroOn = !metroOn; metroNext = now; }
      else {
        uint8_t i = 0;
        while (i < 8 && BPM_PRESETS[i] <= settings.bpm) i++;
        settings.bpm = BPM_PRESETS[i % 8];
        settingsSave();
      }
      break;
    case VIEW_TEMP: case VIEW_HUMIDITY: case VIEW_PRESSURE:
      if (e == EV_LONG) { logSample(); showText(" LOG", 700); }       // take a sample right now
      break;
    case VIEW_WEATHER:
      if (e == EV_LONG) {                                            // fetch now
        if (!wxConfigured() || !settings.wifi_n) { showText(!settings.wifi_n ? "nEt " : " LOC", 1200); break; }
        meshStop();
        showString("....");
        overlayUntil = 0;
        bool ok = wxFetch();
        showText(ok ? "  On" : " Err", 800);
        buzz(ok ? 40 : 200);
        wxPage = WX_TEMP;
        keepAwake();
      } else if (wxFresh()) {
        wxPage = (wxPage + 1) % WX_PAGES;
        if (WX_LABELS[wxPage]) showText(WX_LABELS[wxPage], 600);
      }
      break;
    case VIEW_DATE:
      if (e == EV_LONG) { doSync(); break; }
      // fall through: short press = LED, like the other clock screens
    default:
      if (e == EV_SHORT) { partyMode = false; ledColour = (ledColour + 1) % 8; }
      else partyMode = !partyMode;
      ledSince = now;
  }
}

// --------------------------------------------------------------------- sleep
static bool weatherScreenOn() { return memchr(settings.views, VIEW_WEATHER, settings.views_n) != nullptr; }

static void goToSleep() {
  savedView = viewIndex;
  meshStop();
  displayEnd();
  digitalWrite(PIN_LDR_PWR, LOW);           // light sensor and accessory AUX_PWR off
  Wire.end();
  esp_deep_sleep_enable_gpio_wakeup((1ULL << PIN_BTN_MODE) | (1ULL << PIN_BTN_SET),
                                    ESP_GPIO_WAKEUP_GPIO_LOW);
  // Wake by ourselves for whichever comes first: bell, timer, log sample, time sync.
  long due = -1;
  auto sooner = [&due](long s) { if (s >= 0 && (due < 0 || s < due)) due = s; };
  time_t now = time(nullptr);
  sooner(secondsToNextBell(true));
  if (timerEndMs) sooner((long)((timerEndMs - nowMs()) / 1000) + 1);
  if (seqApp >= 0) sooner((long)((seqEndMs - nowMs()) / 1000) + 1);
  if (settings.log_min && logNextEpoch) sooner((long)(logNextEpoch - now));
  if (timeValid() && wxAutoWanted(weatherScreenOn())) sooner(wxNextEpoch > now ? (long)(wxNextEpoch - now) : 60);
  uint32_t interval = rtcPresent ? settings.resync_h * 3600UL : RESYNC_S_NO_RTC;
  if (settings.wifi_n && interval) {
    long left = interval;
    if (timeValid() && lastSyncEpoch > 0 && now - lastSyncEpoch < (time_t)interval)
      left = interval - (long)(now - lastSyncEpoch);
    sooner(left < 60 ? 60 : left);
  }
  if (due >= 0) esp_sleep_enable_timer_wakeup((uint64_t)(due < 1 ? 1 : due) * 1000000ULL);
  Serial.flush();
  esp_deep_sleep_start();
}

// --------------------------------------------------------------------- setup
void setup() {
  displayBegin();                           // first: get the outputs into a known state
  Serial.begin(115200);
  settingsLoad();

  pinMode(PIN_CHRG, INPUT_PULLUP);
  pinMode(PIN_LDR_PWR, OUTPUT);
  digitalWrite(PIN_LDR_PWR, HIGH);
  analogSetAttenuation(ADC_11db);
  buttonBegin(btnMode);
  buttonBegin(btnSet);

  setenv("TZ", settings.tz, 1);
  tzset();

  Wire.begin(PIN_SDA, PIN_SCL, 100000);
  rtcPresent = rtcDetect();
  time_t t;
  if (rtcPresent && rtcRead(t)) setSystemTime(t);   // DS3231 is the source of truth
  envBegin();
  fsBegin();
  if (settings.log_min && !logNextEpoch) logNextEpoch = time(nullptr) + (time_t)settings.log_min * 60;

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  Serial.printf("Watch V1 fw %s, wake cause %d, rtc %d, env %d, time valid %d\n",
                FW_VERSION, (int)cause, rtcPresent, envAddr != 0, timeValid());

  viewIndex = savedView < totalViews() ? savedView : 0;

  if (cause == ESP_SLEEP_WAKEUP_TIMER) {    // woke by ourselves: do the chore, stay dark unless it is for the wearer
    setBrightness(0);
    bool forWearer = bellCheck() | timerCheck() | seqCheck();
    if (settings.log_min && logNextEpoch && time(nullptr) >= logNextEpoch) {
      delay(60);                            // let the light sensor and BME280 settle
      logSample();
    }
    uint32_t interval = rtcPresent ? settings.resync_h * 3600UL : RESYNC_S_NO_RTC;
    if (!forWearer && settings.wifi_n && interval &&
        (!timeValid() || time(nullptr) - lastSyncEpoch >= (time_t)interval - 5)) timeSync();
    if (!forWearer && wxAutoWanted(weatherScreenOn()) && timeValid() && time(nullptr) >= wxNextEpoch) wxFetch();
    if (!forWearer) goToSleep();
  }

  setBrightness(ambientBrightness());
  if (cause != ESP_SLEEP_WAKEUP_GPIO && cause != ESP_SLEEP_WAKEUP_TIMER) {   // power-on or reset
    if (!timeValid() && settings.wifi_n) doSync();
    viewIndex = 0;
    if (settings.hello[0]) showText(settings.hello, 1800);     // the owner's greeting
  }
  if (batteryMilliVolts() < VBAT_EMPTY_MV && !isCharging()) showText(" Lo ", 700);
  lastActivity = millis();
}

// ---------------------------------------------------------------------- loop
void loop() {
  uint32_t now = millis();
  Event em = buttonPoll(btnMode);
  Event es = buttonPoll(btnSet);
  if (em != EV_NONE || es != EV_NONE) {
    lastActivity = now;
    overlayUntil = 0;
    if (settings.haptic) buzz(em == EV_LONG || es == EV_LONG ? 60 : 25);
    if (streamMs) {                                           // tell a listening computer which button it was
      char msg[64];
      snprintf(msg, sizeof(msg), "{\"ev\":\"btn\",\"b\":\"%s\",\"t\":\"%s\"}",
               em != EV_NONE ? "mode" : "set", (em == EV_LONG || es == EV_LONG) ? "long" : "short");
      linkEvent(msg);
    }
  }

  // Both buttons held for 2 s: link mode (Bluetooth on for Watch Studio).
  if (btnMode.down && btnSet.down && !btnMode.ignore && !btnSet.ignore &&
      now - max(btnMode.downAt, btnSet.downAt) > 2000) {
    btnMode.ignore = btnSet.ignore = true;
    setMode = false;
    linkMode = true;
    bleStart();
    showText("LInk", 1500);
    lastActivity = millis();
  } else if (setMode) {
    if (em == EV_LONG) saveSetMode();
    else if (em == EV_SHORT) {
      field++;
      if (field >= F_COUNT) saveSetMode();
    }
    if (es == EV_SHORT) bumpField();
    if (btnSet.down && btnSet.longFired && now - lastRepeat > 120) {   // auto-repeat
      lastRepeat = now;
      lastActivity = now;
      bumpField();
    }
    if (now - lastActivity > SET_TIMEOUT_MS) setMode = false;          // cancel
  } else {
    if (em == EV_SHORT) {
      viewIndex = (viewIndex + 1) % totalViews();
      msgStart = now;
      metroOn = false;
      pitchNow.ok = false;
      wxPage = WX_TEMP;
    }
    if (em == EV_LONG) enterSetMode();
    if (es != EV_NONE) onSet(es);
  }

  // Things that happen on their own.
  static uint32_t lastSecond = 0;
  if (now - lastSecond >= 1000) {
    lastSecond = now;
    bellCheck();
    timerCheck();
    seqCheck();
    if (settings.log_min && logNextEpoch && time(nullptr) >= logNextEpoch) logSample();
  }
  if (metroOn && (int32_t)(now - metroNext) >= 0) {
    metroNext += 60000UL / settings.bpm;
    buzz(30);
    lastActivity = now;
  }
  if (patLeft && (int32_t)(now - patNext) >= 0) {
    patLeft--;
    buzz(patOn);
    patNext = now + patOn + patOff;
  }

  bool usbHold = settings.usb_awake && usbConnected();
  usbLoop();
  bleLoop();
  streamLoop();
  meshLoop(linkMode || usbHold || view() == VIEW_MESH);
  if (rebootWanted) { delay(200); ESP.restart(); }

  if (ledActive() && now - ledSince > LED_MAX_MS) { ledColour = 0; partyMode = false; }

  if (setMode) renderSet();
  else if (now < overlayUntil) showString(overlay);
  else renderView();
  if (setMode || now < overlayUntil) ledOverride = 0xFF;
  applyLed();

  // Haptic motor is on a shift-register output, which /OE PWM would chop, so
  // run at full duty while it is buzzing.
  if (auxBits & AUX_HAPTIC) {
    if ((int32_t)(now - hapticUntil) >= 0) auxBits = auxBits & ~AUX_HAPTIC;
    setBrightness(255);
  } else {
    static uint32_t lastLdr = 0;
    static uint8_t level = 128;
    if (now - lastLdr > 200) { lastLdr = now; level = ambientBrightness(); }
    setBrightness(level);
  }

  // Sleep when nothing needs the display.
  uint8_t v = view();
  bool appRunning = (v == VIEW_STOPWATCH && swRunning) || (v == VIEW_TIMER && timerEndMs) ||
                    v == VIEW_MESH || v == VIEW_TUNER || (v == VIEW_METRONOME && metroOn) ||
                    (v == VIEW_APP && seqApp >= 0 && seqApp == appIndex());
  bool busy = setMode || ledActive() || btnMode.down || btnSet.down || patLeft || now < overlayUntil ||
              (auxBits & AUX_HAPTIC);
  uint32_t idle = now - lastActivity;
  bool stay = busy || usbHold || (appRunning && idle < APP_AWAKE_MS) || (linkMode && idle < LINK_AWAKE_MS);
  if (!stay && idle > settings.awake_s * 1000UL) goToSleep();
  delay(5);
}
