// Where the watch gets the time from over Wi-Fi.
//
// Two providers, chosen by the "time_src" setting:
//   "ntp"  - classic network time (setting "ntp" = server name). The default.
//   "http" - GET the URL in "time_url", which must answer with JSON, and read
//            Unix time (seconds or milliseconds) from the field named by
//            "time_key". Dots walk into nested objects: "data.epoch".
//            Falls back to NTP if the API does not answer. The default is the
//            project's service (config.h SERVICE_URL), field "unix".
//
// To add another provider (a school API, a home server, ...), write a function
// like the two below and add one line to timeFetch(). Nothing else in the
// firmware cares where the time came from.
#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "esp_sntp.h"
#include "types.h"

// Joins the strongest saved network that is in range. One scan (about 2 s)
// costs far less battery than trying every saved network in turn.
static bool wifiConnect() {
  if (settings.wifi_n == 0) return false;
  WiFi.mode(WIFI_STA);
  int found = WiFi.scanNetworks();
  int best = -1, bestRssi = -1000;
  for (int i = 0; i < found; i++) {
    for (uint8_t k = 0; k < settings.wifi_n; k++) {
      if (WiFi.SSID(i) == settings.wifi[k].ssid && WiFi.RSSI(i) > bestRssi) {
        best = k;
        bestRssi = WiFi.RSSI(i);
      }
    }
  }
  WiFi.scanDelete();
  if (best < 0) return false;
  WiFi.begin(settings.wifi[best].ssid, settings.wifi[best].pass);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) delay(50);
  return WiFi.status() == WL_CONNECTED;
}

static void wifiOff() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

static bool timeFromNtp(time_t &out) {
  sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
  configTzTime(settings.tz, settings.ntp, "time.google.com");
  uint32_t start = millis();
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && millis() - start < NTP_TIMEOUT_MS) delay(50);
  bool ok = sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED;
  esp_sntp_stop();
  if (ok) out = time(nullptr);
  return ok;
}

static bool timeFromHttp(time_t &out) {
  if (!settings.time_url[0]) return false;
  HTTPClient http;
  WiFiClientSecure tls;
  WiFiClient plain;
  tls.setInsecure();              // no certificate store on the watch; fine for a clock, not for secrets
  http.setTimeout(HTTP_TIMEOUT_MS);
  bool https = strncmp(settings.time_url, "https", 5) == 0;
  if (!(https ? http.begin(tls, settings.time_url) : http.begin(plain, settings.time_url))) return false;
  bool ok = false;
  if (http.GET() == 200) {
    JsonDocument doc;
    if (!deserializeJson(doc, http.getString())) {
      JsonVariantConst v = doc.as<JsonVariantConst>();
      char key[sizeof(settings.time_key)];
      strlcpy(key, settings.time_key[0] ? settings.time_key : "unixtime", sizeof(key));
      for (char *part = strtok(key, "."); part; part = strtok(nullptr, ".")) v = v[part];
      double t = v.as<double>();
      if (t > 1e12) t /= 1000.0;  // milliseconds
      if (t > 1735689600.0) {     // after 2025-01-01: plausible
        out = (time_t)t;
        ok = true;
      }
    }
  }
  http.end();
  return ok;
}

// The web API first when chosen, NTP if it does not answer (service down, busy, or blocked).
static bool timeFetch(time_t &out) {
  if (strcmp(settings.time_src, "http") == 0 && timeFromHttp(out)) return true;
  return timeFromNtp(out);
}

// Connects, fetches the time, sets the clocks, disconnects. Blocking (the
// display keeps refreshing: its multiplexing runs from a timer).
static bool timeSync() {
  bool ok = false;
  time_t t = 0;
  if (wifiConnect() && timeFetch(t)) {
    setClock(t);
    lastSyncEpoch = t;
    ok = true;
  }
  wifiOff();
  Serial.printf("time sync (%s): %s\n", settings.time_src, ok ? "ok" : "failed");
  return ok;
}
