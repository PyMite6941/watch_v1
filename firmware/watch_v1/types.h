// Shared types and the functions the modules call in each other.
// Types live in a header because the Arduino builder inserts function
// prototypes above anything declared in the .ino itself.
#pragma once
#include <Arduino.h>
#include "config.h"

struct Button {
  uint8_t  pin;
  bool     down;        // debounced state
  bool     ignore;      // press that woke us up: do not treat it as a command
  bool     longFired;
  uint32_t changedAt;
  uint32_t downAt;
};
enum Event { EV_NONE, EV_SHORT, EV_LONG };
enum Field { F_HOUR, F_MINUTE, F_MONTH, F_DAY, F_YEAR, F_COUNT };

// The screens MODE cycles through. Order here = order of VIEW_NAMES in settings.h.
enum View {
  VIEW_TIME, VIEW_SECONDS, VIEW_DATE, VIEW_BATTERY,
  VIEW_STOPWATCH, VIEW_TIMER, VIEW_COUNTER, VIEW_DICE, VIEW_PERIOD, VIEW_MESH,
  VIEW_TEMP, VIEW_HUMIDITY, VIEW_PRESSURE,      // need the environment pack (BME280)
  VIEW_TUNER,                                   // needs the sound pack (microphone)
  VIEW_METRONOME,
  VIEW_COUNT,
  VIEW_APP = 200                                // a custom app from /apps (see apps.h); never stored in settings
};

struct WifiNet {
  char ssid[33];
  char pass[65];
};

// Everything the owner can change. Stored as JSON in flash (NVS).
struct Settings {
  char     name[17];            // shown to other watches, Bluetooth name suffix
  char     tz[48];              // POSIX TZ string
  bool     h24;                 // 24-hour clock
  uint8_t  awake_s;             // seconds the display stays on
  bool     auto_bright;         // follow the light sensor
  uint8_t  bright_min;          // 1-255
  uint8_t  bright_max;          // 1-255 (the fixed brightness when auto_bright is off)
  bool     haptic;              // vibrate on button presses
  uint8_t  views[VIEW_COUNT];   // enabled screens, in order
  uint8_t  views_n;
  WifiNet  wifi[MAX_WIFI];
  uint8_t  wifi_n;
  char     time_src[8];         // "ntp" or "http"
  char     ntp[40];
  char     time_url[128];       // for "http": a URL that returns JSON
  char     time_key[24];        // ... and the field holding Unix time, e.g. "unixtime" or "data.epoch"
  uint16_t resync_h;            // hours between automatic time syncs (0 = never)
  bool     usb_awake;           // stay awake while a computer is connected
  char     pin[9];              // optional PIN for Bluetooth configuration ("" = none)
  bool     mesh;                // ESP-NOW group messaging
  uint8_t  mesh_ch;             // Wi-Fi channel 1-13 the group uses
  char     mesh_group[17];
  uint16_t timer_s;             // countdown timer length
  uint8_t  dice;                // dice sides (2-99)
  uint16_t alerts[MAX_ALERTS];  // bell schedule: minutes after midnight
  uint8_t  alerts_n;
  uint8_t  alert_days;          // bit0 = Sunday ... bit6 = Saturday
  uint8_t  alert_warn_min;      // extra short buzz this many minutes before each bell (0 = off)
  uint16_t log_min;             // record an environment sample every N minutes (0 = off)
  bool     fahrenheit;
  uint16_t tune_a4;             // tuner reference pitch, Hz
  uint16_t bpm;                 // metronome
  char     hello[5];            // four characters shown when the watch is switched on ("" = nothing)
};

extern Settings settings;

// ---- implemented in watch_v1.ino
void     buzz(uint16_t ms);
void     showText(const char *text, uint16_t ms);
uint16_t batteryMilliVolts();
uint8_t  batteryPercent(uint16_t mv);
bool     isCharging();
bool     timeValid();
void     setClock(time_t utc);      // sets the system clock and the DS3231
void     keepAwake();
void     buzzPattern(uint8_t count, uint16_t onMs, uint16_t offMs);
bool     usbConnected();
void     linkEvent(const char *json);   // sends an event line to a listening computer, if any

#include <sys/time.h>
static inline int64_t nowMs() {     // wall-clock milliseconds; keeps counting through deep sleep
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

extern bool   rtcPresent;
extern time_t lastSyncEpoch;
