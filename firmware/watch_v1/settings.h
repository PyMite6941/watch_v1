// Settings: defaults, JSON <-> struct, storage in flash.
//
// The JSON form is the public interface - it is what the configurator sends
// and what docs/CONFIGURATION.md documents. Unknown keys are ignored and every
// value is range-checked, so a bad message cannot brick the watch.
#pragma once
#include <ArduinoJson.h>
#include <Preferences.h>
#include "types.h"

#if __has_include("secrets.h")
#include "secrets.h"
#endif

Settings settings;

static const char *const VIEW_NAMES[VIEW_COUNT] = {
  "time", "seconds", "date", "battery", "stopwatch", "timer", "counter", "dice", "period", "mesh",
  "temp", "humidity", "pressure", "tuner", "metronome"
};

static void copyStr(char *dst, size_t size, const char *src) {
  strlcpy(dst, src ? src : "", size);
}

static void settingsDefaults() {
  memset(&settings, 0, sizeof(settings));
  copyStr(settings.name, sizeof(settings.name), DEFAULT_NAME);
  copyStr(settings.tz, sizeof(settings.tz), DEFAULT_TZ);
  settings.h24 = true;
  settings.awake_s = DEFAULT_AWAKE_S;
  settings.auto_bright = true;
  settings.bright_min = 20;
  settings.bright_max = 255;
  settings.haptic = true;
  const uint8_t views[] = { VIEW_TIME, VIEW_DATE, VIEW_BATTERY, VIEW_STOPWATCH, VIEW_TIMER };
  memcpy(settings.views, views, sizeof(views));
  settings.views_n = sizeof(views);
  copyStr(settings.time_src, sizeof(settings.time_src), "ntp");
  copyStr(settings.ntp, sizeof(settings.ntp), DEFAULT_NTP);
  settings.resync_h = DEFAULT_RESYNC_H;
  settings.usb_awake = true;
  settings.mesh = false;
  settings.mesh_ch = DEFAULT_MESH_CH;
  copyStr(settings.mesh_group, sizeof(settings.mesh_group), DEFAULT_MESH_GROUP);
  settings.timer_s = DEFAULT_TIMER_S;
  settings.dice = DEFAULT_DICE;
  settings.alert_days = 0x3E;          // Monday-Friday
  settings.tune_a4 = 440;
  settings.bpm = 100;
#ifdef WIFI_SSID
  copyStr(settings.wifi[0].ssid, sizeof(settings.wifi[0].ssid), WIFI_SSID);
  copyStr(settings.wifi[0].pass, sizeof(settings.wifi[0].pass), WIFI_PASSWORD);
  settings.wifi_n = 1;
#endif
}

// withSecrets: include Wi-Fi passwords and the PIN (for storage only - they
// are never sent back over USB or Bluetooth).
static void settingsToJson(JsonObject o, bool withSecrets) {
  o["name"] = settings.name;
  o["tz"] = settings.tz;
  o["h24"] = settings.h24;
  o["awake_s"] = settings.awake_s;
  o["auto_bright"] = settings.auto_bright;
  o["bright_min"] = settings.bright_min;
  o["bright_max"] = settings.bright_max;
  o["haptic"] = settings.haptic;
  JsonArray views = o["views"].to<JsonArray>();
  for (uint8_t i = 0; i < settings.views_n; i++) views.add(VIEW_NAMES[settings.views[i]]);
  JsonArray wifi = o["wifi"].to<JsonArray>();
  for (uint8_t i = 0; i < settings.wifi_n; i++) {
    JsonObject n = wifi.add<JsonObject>();
    n["ssid"] = settings.wifi[i].ssid;
    if (withSecrets) n["pass"] = settings.wifi[i].pass;
  }
  o["time_src"] = settings.time_src;
  o["ntp"] = settings.ntp;
  o["time_url"] = settings.time_url;
  o["time_key"] = settings.time_key;
  o["resync_h"] = settings.resync_h;
  o["usb_awake"] = settings.usb_awake;
  if (withSecrets) o["pin"] = settings.pin;
  else o["pin_set"] = settings.pin[0] != 0;
  o["mesh"] = settings.mesh;
  o["mesh_ch"] = settings.mesh_ch;
  o["mesh_group"] = settings.mesh_group;
  o["timer_s"] = settings.timer_s;
  o["dice"] = settings.dice;
  JsonArray alerts = o["alerts"].to<JsonArray>();
  for (uint8_t i = 0; i < settings.alerts_n; i++) {
    char buf[6];
    snprintf(buf, sizeof(buf), "%02u:%02u", settings.alerts[i] / 60, settings.alerts[i] % 60);
    alerts.add(buf);
  }
  o["alert_days"] = settings.alert_days;
  o["alert_warn_min"] = settings.alert_warn_min;
  o["log_min"] = settings.log_min;
  o["fahrenheit"] = settings.fahrenheit;
  o["tune_a4"] = settings.tune_a4;
  o["bpm"] = settings.bpm;
}

static long clampLong(JsonVariantConst v, long lo, long hi) {
  long x = v.as<long>();
  return x < lo ? lo : (x > hi ? hi : x);
}

// Applies whichever keys are present. Returns false if nothing was recognised.
static bool settingsFromJson(JsonObjectConst o, bool withSecrets) {
  bool any = false;
#define STR(key, field) if (o[key].is<const char *>()) { copyStr(settings.field, sizeof(settings.field), o[key]); any = true; }
#define BOOL(key, field) if (o[key].is<bool>()) { settings.field = o[key].as<bool>(); any = true; }
#define NUM(key, field, lo, hi) if (o[key].is<long>()) { settings.field = clampLong(o[key], lo, hi); any = true; }
  STR("name", name)
  STR("tz", tz)
  BOOL("h24", h24)
  NUM("awake_s", awake_s, 3, 60)
  BOOL("auto_bright", auto_bright)
  NUM("bright_min", bright_min, 1, 255)
  NUM("bright_max", bright_max, 1, 255)
  BOOL("haptic", haptic)
  STR("time_src", time_src)
  STR("ntp", ntp)
  STR("time_url", time_url)
  STR("time_key", time_key)
  NUM("resync_h", resync_h, 0, 2000)
  BOOL("usb_awake", usb_awake)
  STR("pin", pin)
  BOOL("mesh", mesh)
  NUM("mesh_ch", mesh_ch, 1, 13)
  STR("mesh_group", mesh_group)
  NUM("timer_s", timer_s, 5, 5999)
  NUM("dice", dice, 2, 99)
  NUM("alert_days", alert_days, 0, 127)
  NUM("alert_warn_min", alert_warn_min, 0, 30)
  NUM("log_min", log_min, 0, 1440)
  BOOL("fahrenheit", fahrenheit)
  NUM("tune_a4", tune_a4, 400, 480)
  NUM("bpm", bpm, 30, 240)
#undef STR
#undef BOOL
#undef NUM
  if (settings.bright_min > settings.bright_max) settings.bright_min = settings.bright_max;

  if (o["views"].is<JsonArrayConst>()) {
    uint8_t n = 0;
    for (JsonVariantConst v : o["views"].as<JsonArrayConst>()) {
      const char *name = v.as<const char *>();
      for (uint8_t id = 0; name && id < VIEW_COUNT && n < VIEW_COUNT; id++) {
        if (strcmp(name, VIEW_NAMES[id]) == 0 && !memchr(settings.views, id, n)) settings.views[n++] = id;
      }
    }
    if (n == 0) settings.views[n++] = VIEW_TIME;        // never leave the watch with no screen
    settings.views_n = n;
    any = true;
  }
  if (o["alerts"].is<JsonArrayConst>()) {
    uint8_t n = 0;
    for (JsonVariantConst v : o["alerts"].as<JsonArrayConst>()) {
      unsigned h, m;
      const char *s = v.as<const char *>();
      if (s && n < MAX_ALERTS && sscanf(s, "%u:%u", &h, &m) == 2 && h < 24 && m < 60) settings.alerts[n++] = h * 60 + m;
    }
    settings.alerts_n = n;
    any = true;
  }
  // The Wi-Fi list is only replaced wholesale from storage; over the link it
  // is edited with the wifi_add / wifi_del commands so passwords stay put.
  if (withSecrets && o["wifi"].is<JsonArrayConst>()) {
    uint8_t n = 0;
    for (JsonObjectConst w : o["wifi"].as<JsonArrayConst>()) {
      if (n >= MAX_WIFI || !w["ssid"].is<const char *>()) continue;
      copyStr(settings.wifi[n].ssid, sizeof(settings.wifi[n].ssid), w["ssid"]);
      copyStr(settings.wifi[n].pass, sizeof(settings.wifi[n].pass), w["pass"] | "");
      n++;
    }
    settings.wifi_n = n;
    any = true;
  }
  return any;
}

static void settingsSave() {
  JsonDocument doc;
  settingsToJson(doc.to<JsonObject>(), true);
  String out;
  serializeJson(doc, out);
  Preferences prefs;
  prefs.begin("watch", false);
  prefs.putString("cfg", out);
  prefs.end();
}

static void settingsLoad() {
  settingsDefaults();
  Preferences prefs;
  prefs.begin("watch", true);
  String in = prefs.getString("cfg", "");
  prefs.end();
  JsonDocument doc;
  if (in.length() && !deserializeJson(doc, in)) settingsFromJson(doc.as<JsonObjectConst>(), true);
}

static void settingsErase() {
  Preferences prefs;
  prefs.begin("watch", false);
  prefs.clear();
  prefs.end();
  settingsDefaults();
}

static bool wifiAdd(const char *ssid, const char *pass) {
  if (!ssid || !ssid[0]) return false;
  uint8_t i = 0;
  while (i < settings.wifi_n && strcmp(settings.wifi[i].ssid, ssid) != 0) i++;
  if (i == MAX_WIFI) return false;                       // list full
  copyStr(settings.wifi[i].ssid, sizeof(settings.wifi[i].ssid), ssid);
  copyStr(settings.wifi[i].pass, sizeof(settings.wifi[i].pass), pass);
  if (i == settings.wifi_n) settings.wifi_n++;
  return true;
}

static bool wifiDel(const char *ssid) {
  for (uint8_t i = 0; ssid && i < settings.wifi_n; i++) {
    if (strcmp(settings.wifi[i].ssid, ssid) == 0) {
      for (uint8_t j = i; j + 1 < settings.wifi_n; j++) settings.wifi[j] = settings.wifi[j + 1];
      settings.wifi_n--;
      return true;
    }
  }
  return false;
}
