// Weather from the project's web service (setting "wx_url", the same service
// that can supply the time).
//
//   GET <wx_url>?include=weather,forecast,sun,air&days=1&lat=..&lon=..  (or &city=..)
//       [&units=imperial when the watch shows Fahrenheit]
//
// The watch keeps only a few numbers from the reply, in memory that survives
// deep sleep, and fetches again every "wx_min" minutes (the service refreshes
// weather every 10 minutes, so more often is pointless). A 429 reply means the
// service is busy: the watch waits "retry_after" seconds before trying again.
//
// The weather screen pages through the numbers with SET; holding SET fetches now.
#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "types.h"

#define WX_NONE -32768

struct Weather {
  time_t  at;                 // when it was fetched (0 = never)
  int16_t temp;               // now, whole degrees in the units it was fetched in
  int16_t feels;
  uint8_t code;               // WMO weather code (0 clear ... 99 thunderstorm)
  bool    isDay;
  int16_t high, low;          // today
  uint8_t rain;               // today's chance of rain, %
  int16_t sunrise, sunset;    // minutes after local midnight at that place; WX_NONE = unknown
  int16_t aqi;                // US air quality index; WX_NONE = unknown
  bool    imperial;
};

RTC_DATA_ATTR Weather wx = {};
RTC_DATA_ATTR time_t  wxNextEpoch = 0;   // earliest time for the next automatic fetch

// Four seven-segment characters for a WMO weather code. The display has no
// K M V W X Z, hence "SHOr" for showers and "drIP" for drizzle.
static const char *wxWord(uint8_t code) {
  if (code <= 1) return "CLr ";
  if (code == 2) return "PArt";
  if (code == 3) return "CLdy";
  if (code == 45 || code == 48) return "FOG ";
  if (code >= 51 && code <= 57) return "drIP";
  if (code == 66 || code == 67) return "ICE ";
  if (code >= 61 && code <= 65) return "rAIn";
  if ((code >= 71 && code <= 77) || code == 85 || code == 86) return "SnO ";
  if (code >= 80 && code <= 82) return "SHOr";
  if (code >= 95) return "StOr";
  return "----";
}

static bool wxConfigured() {
  return settings.wx_url[0] && (settings.wx_city[0] || settings.wx_lat != 0 || settings.wx_lon != 0);
}

// "2026-11-15T07:18" -> 438
static int16_t wxClock(const char *iso) {
  const char *t = iso ? strchr(iso, 'T') : nullptr;
  unsigned h, m;
  if (!t || sscanf(t + 1, "%u:%u", &h, &m) != 2 || h > 23 || m > 59) return WX_NONE;
  return h * 60 + m;
}

static int16_t wxRound(JsonVariantConst v) {
  return v.is<float>() || v.is<long>() ? (int16_t)lroundf(v.as<float>()) : WX_NONE;
}

// Percent-encodes a city name for the query string.
static void wxAppendEscaped(String &url, const char *s) {
  for (; *s; s++) {
    char c = *s;
    if (isalnum((unsigned char)c) || c == '-' || c == '.' || c == '_') url += c;
    else if (c == ' ') url += "%20";
    else { char hex[4]; snprintf(hex, sizeof(hex), "%%%02X", (uint8_t)c); url += hex; }
  }
}

// Needs Wi-Fi already connected. Returns the HTTP status (200 = ok), or -1.
// With setTime, also corrects the clock from the reply's "unix" field.
static int wxFetchConnected(bool setTime) {
  if (!wxConfigured()) return -1;
  String url = settings.wx_url;
  url += strchr(settings.wx_url, '?') ? '&' : '?';
  url += "include=weather,forecast,sun,air&days=1";
  if (settings.wx_city[0]) { url += "&city="; wxAppendEscaped(url, settings.wx_city); }
  else { url += "&lat=" + String(settings.wx_lat, 2) + "&lon=" + String(settings.wx_lon, 2); }
  if (settings.fahrenheit) url += "&units=imperial";

  HTTPClient http;
  WiFiClientSecure tls;
  WiFiClient plain;
  tls.setInsecure();                 // no certificate store on the watch; the reply holds nothing secret
  http.setTimeout(HTTP_TIMEOUT_MS);
  bool https = url.startsWith("https");
  if (!(https ? http.begin(tls, url) : http.begin(plain, url))) return -1;
  int code = http.GET();
  time_t now = time(nullptr);
  if (code == 200) {
    JsonDocument filter;              // keep only what the watch uses
    filter["unix"] = true;
    filter["status"] = true;
    filter["weather"]["temp"] = true;
    filter["weather"]["feels_like"] = true;
    filter["weather"]["code"] = true;
    filter["weather"]["is_day"] = true;
    filter["forecast"][0]["high"] = true;
    filter["forecast"][0]["low"] = true;
    filter["forecast"][0]["rain_chance"] = true;
    filter["sun"]["sunrise"] = true;
    filter["sun"]["sunset"] = true;
    filter["air"]["us_aqi"] = true;
    JsonDocument doc;
    if (!deserializeJson(doc, http.getString(), DeserializationOption::Filter(filter))) {
      double unix = doc["unix"] | 0.0;
      if (setTime && unix > 1735689600.0) {
        setClock((time_t)unix);
        lastSyncEpoch = (time_t)unix;
        now = (time_t)unix;
      }
      if (doc["weather"]["temp"].is<float>() || doc["weather"]["temp"].is<long>()) {
        wx.temp = wxRound(doc["weather"]["temp"]);
        wx.feels = wxRound(doc["weather"]["feels_like"]);
        wx.code = doc["weather"]["code"] | 255;
        wx.isDay = doc["weather"]["is_day"] | true;
        JsonVariantConst today = doc["forecast"][0];
        wx.high = wxRound(today["high"]);
        wx.low = wxRound(today["low"]);
        wx.rain = today["rain_chance"] | 0;
        wx.sunrise = wxClock(doc["sun"]["sunrise"]);
        wx.sunset = wxClock(doc["sun"]["sunset"]);
        wx.aqi = wxRound(doc["air"]["us_aqi"]);
        wx.imperial = settings.fahrenheit;
        wx.at = now;
      } else {
        code = -1;                    // "degraded": the weather part could not be fetched
      }
    } else {
      code = -1;
    }
    wxNextEpoch = now + (time_t)(settings.wx_min ? settings.wx_min : 60) * 60;
  } else if (code == 429) {
    long wait = 600;
    JsonDocument doc;
    if (!deserializeJson(doc, http.getString())) wait = max(60L, (long)(doc["retry_after"] | 600));
    wxNextEpoch = now + wait;
  } else {
    wxNextEpoch = now + 15 * 60;      // offline or the service is down: try again later, not straight away
  }
  http.end();
  Serial.printf("weather fetch: %d\n", code);
  return code;
}

// Connects, fetches, disconnects. Blocking: a few seconds.
static bool wxFetch() {
  if (!wxConfigured()) return false;
  bool ok = false;
  if (wifiConnect()) ok = wxFetchConnected(strcmp(settings.time_src, "http") == 0) == 200;
  else wxNextEpoch = time(nullptr) + 15 * 60;
  wifiOff();
  return ok;
}

// Weather older than this is not shown.
static bool wxFresh() { return wx.at && time(nullptr) - wx.at < 12 * 3600; }

// Automatic fetches only run while the weather screen is one of the owner's screens.
static bool wxAutoWanted(bool weatherScreenOn) {
  return weatherScreenOn && settings.wx_min && settings.wifi_n && wxConfigured();
}
