// Files on the watch, and custom apps made from them.
//
// The watch has a small flash file system (about 190 kB). Anything can be
// stored there with the fs_* commands, but files in /apps are special: each
// /apps/<name>.json describes one extra screen, which appears after the
// built-in ones when MODE is pressed. They are made in Watch Studio (app/),
// no programming needed. Four kinds:
//
//   {"type":"sequence","title":"LAb1","repeat":false,
//    "steps":[{"label":"StAn","s":120},{"label":"rInS","s":30}]}
//        A chain of timed steps. Buzzes and shows the label at each change.
//        Runs on while the watch sleeps.  SET: start / skip a step.  Hold: stop.
//
//   {"type":"picker","title":"PICk","items":["AnnA","bEn","CArL"],"no_repeat":true}
//   {"type":"picker","title":"PICk","max":28}
//        Random choice from a list of names, or a number 1..max.  SET: pick.
//
//   {"type":"countdown","title":"ExAm","date":"2027-03-15"}
//        Days left ("d 42"), then hours:minutes on the last day.
//
//   {"type":"message","title":"notE","text":"HAPPY bIrtHdAY"}
//        Scrolls the text.  SET: again.
//
// Labels and titles are at most four characters, drawn with the watch's
// seven-segment alphabet (letters such as K, M, V, W, X have no shape).
#pragma once
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "types.h"

#define MAX_APPS       8
#define APP_MAX_ITEMS  32
#define APP_PATH_LEN   48

enum AppType : uint8_t { APP_NONE, APP_SEQUENCE, APP_PICKER, APP_COUNTDOWN, APP_MESSAGE };

struct AppItem {
  char     label[5];
  uint16_t seconds;
};

struct CustomApp {
  AppType  type;
  char     title[5];
  bool     repeat, noRepeat;
  uint8_t  n;                       // steps or items
  AppItem  items[APP_MAX_ITEMS];
  uint16_t max;                     // picker without a list: 1..max
  time_t   date;                    // countdown target (local midnight)
  char     text[97];
};

static bool      fsReady = false;
static char      appFiles[MAX_APPS][APP_PATH_LEN];
static uint8_t   appCount = 0;
static CustomApp app;               // the one currently loaded
static int8_t    appLoaded = -1;    // index into appFiles, -1 = none

// Kept through deep sleep.
RTC_DATA_ATTR static int8_t   seqApp = -1;      // which app's sequence is running
RTC_DATA_ATTR static uint8_t  seqStep = 0;
RTC_DATA_ATTR static int64_t  seqEndMs = 0;     // when the current step ends
RTC_DATA_ATTR static int8_t   pickApp = -1;
RTC_DATA_ATTR static uint32_t pickUsed = 0;     // items already drawn (no_repeat)
RTC_DATA_ATTR static int16_t  pickLast = -1;

static bool fsPathOk(const char *p) {
  return p && p[0] == '/' && strlen(p) < APP_PATH_LEN && !strstr(p, "..") && !strstr(p, "//");
}

// Finds the app files. Cheap enough to do on every wake.
static void appsScan() {
  appCount = 0;
  appLoaded = -1;
  if (!fsReady) return;
  File dir = LittleFS.open("/apps");
  if (!dir || !dir.isDirectory()) return;
  for (File f = dir.openNextFile(); f && appCount < MAX_APPS; f = dir.openNextFile()) {
    String name = f.name();
    if (!f.isDirectory() && name.endsWith(".json") && name.length() + 6 < APP_PATH_LEN) {
      snprintf(appFiles[appCount++], APP_PATH_LEN, "/apps/%s", name.c_str());
    }
  }
  // Alphabetical, so the order on the watch is the order shown in the app.
  for (uint8_t i = 1; i < appCount; i++) {
    for (uint8_t j = i; j > 0 && strcmp(appFiles[j - 1], appFiles[j]) > 0; j--) {
      char tmp[APP_PATH_LEN];
      strcpy(tmp, appFiles[j]);
      strcpy(appFiles[j], appFiles[j - 1]);
      strcpy(appFiles[j - 1], tmp);
    }
  }
  if (seqApp >= appCount) seqApp = -1;
  if (pickApp >= appCount) pickApp = -1;
}

static void fsBegin() {
  fsReady = LittleFS.begin(true);          // formats itself the first time
  if (fsReady && !LittleFS.exists("/apps")) LittleFS.mkdir("/apps");
  appsScan();
}

// Reads and checks one app file. A broken file becomes a screen that says "bAd".
static bool appLoad(int8_t index) {
  if (index == appLoaded) return app.type != APP_NONE;
  memset(&app, 0, sizeof(app));
  appLoaded = index;
  strlcpy(app.title, "bAd ", sizeof(app.title));
  if (index < 0 || index >= appCount) return false;
  File f = LittleFS.open(appFiles[index], "r");
  JsonDocument doc;
  if (!f || deserializeJson(doc, f)) return false;
  const char *type = doc["type"] | "";
  strlcpy(app.title, doc["title"] | "APP ", sizeof(app.title));
  if (!strcmp(type, "sequence")) {
    for (JsonObjectConst s : doc["steps"].as<JsonArrayConst>()) {
      if (app.n >= APP_MAX_ITEMS) break;
      strlcpy(app.items[app.n].label, s["label"] | "", 5);
      app.items[app.n].seconds = constrain((long)(s["s"] | 60), 1L, 65000L);
      app.n++;
    }
    app.repeat = doc["repeat"] | false;
    if (app.n) app.type = APP_SEQUENCE;
  } else if (!strcmp(type, "picker")) {
    for (JsonVariantConst v : doc["items"].as<JsonArrayConst>()) {
      if (app.n >= APP_MAX_ITEMS) break;
      strlcpy(app.items[app.n++].label, v | "", 5);
    }
    app.max = constrain((long)(doc["max"] | 6), 2L, 9999L);
    app.noRepeat = doc["no_repeat"] | false;
    app.type = APP_PICKER;
  } else if (!strcmp(type, "countdown")) {
    int y, m, d;
    if (sscanf(doc["date"] | "", "%d-%d-%d", &y, &m, &d) == 3) {
      struct tm t = {};
      t.tm_year = y - 1900;
      t.tm_mon = m - 1;
      t.tm_mday = d;
      t.tm_isdst = -1;
      app.date = mktime(&t);
      app.type = APP_COUNTDOWN;
    }
  } else if (!strcmp(type, "message")) {
    strlcpy(app.text, doc["text"] | "", sizeof(app.text));
    app.type = APP_MESSAGE;
  }
  if (app.type == APP_NONE) strlcpy(app.title, "bAd ", sizeof(app.title));
  return app.type != APP_NONE;
}

// ---------------------------------------------------------------- sequences
static void seqStart(int8_t index) {
  if (!appLoad(index) || app.type != APP_SEQUENCE) return;
  seqApp = index;
  seqStep = 0;
  seqEndMs = nowMs() + (int64_t)app.items[0].seconds * 1000;
  buzz(200);
  showText(app.items[0].label[0] ? app.items[0].label : app.title, 1500);
}

static void seqStop() { seqApp = -1; }

// Moves a running sequence on when its step is over. True if it buzzed.
static bool seqCheck() {
  if (seqApp < 0 || nowMs() < seqEndMs) return false;
  int8_t showing = appLoaded;
  if (!appLoad(seqApp) || app.type != APP_SEQUENCE) {
    seqApp = -1;
    return false;
  }
  seqStep++;
  if (seqStep >= app.n) {
    if (app.repeat) seqStep = 0;
    else {
      seqApp = -1;
      buzzPattern(4, 200, 120);
      showText("End ", 3000);
      keepAwake();
      if (showing != appLoaded) appLoad(showing);
      return true;
    }
  }
  seqEndMs = nowMs() + (int64_t)app.items[seqStep].seconds * 1000;
  buzzPattern(2, 220, 130);
  showText(app.items[seqStep].label[0] ? app.items[seqStep].label : app.title, 2000);
  keepAwake();
  if (showing != appLoaded) appLoad(showing);
  return true;
}

// ------------------------------------------------------------------ pickers
// Draws for the loaded picker app. Returns false when a no-repeat list has just been used up.
static bool pickNext(int8_t index) {
  if (pickApp != index) {
    pickApp = index;
    pickUsed = 0;
  }
  uint16_t range = app.n ? app.n : app.max;
  bool fresh = true;
  if (app.noRepeat && range <= 32) {
    uint32_t all = range == 32 ? 0xFFFFFFFFu : ((1u << range) - 1);
    if ((pickUsed & all) == all) {           // everyone has had a turn: start over
      pickUsed = 0;
      fresh = false;
    }
    uint16_t pick;
    do { pick = esp_random() % range; } while (pickUsed & (1u << pick));
    pickUsed |= 1u << pick;
    pickLast = pick;
  } else {
    pickLast = esp_random() % range;
  }
  return fresh;
}
