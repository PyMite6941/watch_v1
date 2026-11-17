// Optional small OLED screen on the accessory socket (SSD1306, I2C address 0x3C or 0x3D),
// normally on the Screen Pack (hardware/addons/screen_pack).
//
// The four LED digits stay the main display. The OLED shows the same screen in
// words and pictures, drawn from a screen design: a small JSON file that says
// where the clock, icons, bars and text go (docs/OLED_SCREENS.md). Teachers make
// them in Watch Studio's Screens tab; they live in /oled/ on the watch.
//
// Which design is used for the screen on show ("key"):
//   1. a file in /oled whose "for" is the key ("time", "weather", "app:quiz", "set", "overlay")
//   2. for an app, a file whose "for" is "app" (every app)
//   3. the built-in one for the key (tools/oled/builtin.json), else the built-in "card"
// Only files made for this panel's size are used. Names starting with '_' are ignored.
// A file with "when" ({"from":"07:30","to":"15:00","days":"12345"}, Monday = 1) is used
// only at those times, and wins over one without "when" while it applies. So a teacher
// can have a school-day face and a weekend face for the same screen.
//
// settings.oled picks the panel: 0 off, 1 = 128x32 (0.91"), 2 = 128x64 (0.96"),
// 3 = 64x32 (0.49"). If nothing answers at 0x3C/0x3D it is simply skipped.
// settings.oled_flip turns the picture upside down.
//
// Power: the panel is switched off (command 0xAE) before the watch sleeps, and
// only sent a new picture when a pixel changed, at most 4 times a second.
#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LittleFS.h>
#include "apps.h"
#include "oled_render.h"

#define OLED_MIN_MS    250
#define OLED_MAX_FILES 24
#define OLED_KEY_LEN   24

// From watch_v1.ino.
bool timeValid();
static void localNow(struct tm &out);
static bool oledField(const char *name, char *out, size_t n);   // a {field}'s value now
static void oledKey(char *key, size_t n);                       // which screen is on show

static Adafruit_SSD1306 *oled = nullptr;
static uint8_t  oledKind = 0;                 // settings.oled the panel was started with
static uint8_t  oledBuf[128 * 64 / 8], oledLast[128 * 64 / 8];
static bool     oledLastOk = false;
static uint32_t oledAt = 0;
static uint32_t oledFrame = 0;                // counts renders, so fields can be worked out once per picture

struct OledFile {
  char key[OLED_KEY_LEN];
  char path[APP_PATH_LEN];
  bool timed;                                 // has "when"
  uint16_t from, to;                          // minutes after midnight; equal = all day
  uint8_t days;                               // bit 1 = Monday ... bit 7 = Sunday; 0 = every day
};
static OledFile oledFiles[OLED_MAX_FILES];
static uint8_t  oledFileCount = 0;
static bool     oledScanned = false;

static JsonDocument *oledDoc = nullptr;       // the design in use
static char     oledDocSrc[APP_PATH_LEN + 4] = "";

static char     oledPreview[APP_PATH_LEN] = "";   // oled_show: a design tried out from Watch Studio
static uint32_t oledPreviewUntil = 0;

static int oledW() { return settings.oled == 3 ? 64 : 128; }
static int oledH() { return settings.oled == 2 ? 64 : 32; }

static uint8_t oledFind() {
  for (uint8_t a = 0x3C; a <= 0x3D; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) return a;
  }
  return 0;
}

// Call after Wire.begin(), only when the wearer will look at the watch.
static void oledBegin() {
  if (oled || settings.oled == 0) return;
  uint8_t addr = oledFind();
  if (!addr) return;
  oled = new Adafruit_SSD1306(oledW(), oledH(), &Wire, -1);
  // periphBegin = false: Wire is already running on the watch's pins.
  if (!oled->begin(SSD1306_SWITCHCAPVCC, addr, false, false)) { delete oled; oled = nullptr; return; }
  oledKind = settings.oled;
  oled->clearDisplay();
  oled->display();
  oledLastOk = false;
}

static void oledForget() {                    // after the files or the panel size changed
  oledScanned = false;
  oledDocSrc[0] = 0;
  oledLastOk = false;
}

// "07:30" -> 450. Anything else -> 0.
static uint16_t oledMinutes(const char *s) {
  int h = 0, m = 0;
  if (sscanf(s, "%d:%d", &h, &m) != 2 || h < 0 || h > 24 || m < 0 || m > 59) return 0;
  return (uint16_t)((h * 60 + m) % 1440);
}

// Finds the screen files made for this panel.
static void oledScan() {
  oledScanned = true;
  oledFileCount = 0;
  if (!fsReady) return;
  File dir = LittleFS.open("/oled");
  if (!dir || !dir.isDirectory()) return;
  JsonDocument filter;
  filter["for"] = true;
  filter["w"] = true;
  filter["h"] = true;
  filter["when"] = true;
  for (File f = dir.openNextFile(); f && oledFileCount < OLED_MAX_FILES; f = dir.openNextFile()) {
    String name = f.name();
    if (f.isDirectory() || !name.endsWith(".json") || name[0] == '_' || name.length() + 6 >= APP_PATH_LEN) continue;
    JsonDocument doc;
    if (deserializeJson(doc, f, DeserializationOption::Filter(filter))) continue;
    const char *key = doc["for"] | "";
    if (!*key || strlen(key) >= OLED_KEY_LEN || (doc["w"] | 0) != oledW() || (doc["h"] | 0) != oledH()) continue;
    OledFile &o = oledFiles[oledFileCount++];
    strlcpy(o.key, key, sizeof(o.key));
    snprintf(o.path, sizeof(o.path), "/oled/%s", name.c_str());
    JsonVariantConst when = doc["when"];
    o.timed = when.is<JsonObjectConst>();
    o.from = oledMinutes(when["from"] | "");
    o.to = oledMinutes(when["to"] | "");
    o.days = 0;
    for (const char *d = when["days"] | ""; *d; d++) if (*d >= '1' && *d <= '7') o.days |= 1 << (*d - '0');
  }
}

// Is a timed file's "when" true now? (No clock yet: no.)
static bool oledWhenNow(const OledFile &o) {
  if (!timeValid()) return false;
  struct tm t;
  localNow(t);
  int wd = t.tm_wday ? t.tm_wday : 7;
  if (o.days && !(o.days & (1 << wd))) return false;
  int m = t.tm_hour * 60 + t.tm_min;
  if (o.from == o.to) return true;
  return o.from < o.to ? (m >= o.from && m < o.to) : (m >= o.from || m < o.to);
}

// A timed file whose "when" applies now wins; otherwise the first one without "when".
static const char *oledFileFor(const char *key) {
  const char *plain = nullptr;
  for (uint8_t i = 0; i < oledFileCount; i++) {
    const OledFile &o = oledFiles[i];
    if (strcmp(o.key, key)) continue;
    if (!o.timed) { if (!plain) plain = o.path; }
    else if (oledWhenNow(o)) return o.path;
  }
  return plain;
}
static int oledBuiltinFor(const char *key) {
  for (size_t i = 0; i < OLED_BUILTIN_COUNT; i++)
    if (OLED_BUILTIN[i].w == oledW() && OLED_BUILTIN[i].h == oledH() && !strcmp(OLED_BUILTIN[i].key, key)) return i;
  return -1;
}

// Loads the design from "f:<path>" or "b:<index>" unless it is the one already loaded.
static bool oledLoad(const char *src) {
  if (oledDoc && !strcmp(src, oledDocSrc)) return true;
  if (!oledDoc) oledDoc = new JsonDocument;
  oledDoc->clear();
  oledDocSrc[0] = 0;
  DeserializationError err;
  if (src[0] == 'f') {
    File f = LittleFS.open(src + 2, "r");
    if (!f) return false;
    err = deserializeJson(*oledDoc, f);
  } else {
    err = deserializeJson(*oledDoc, OLED_BUILTIN[atoi(src + 2)].json);
  }
  if (err) return false;
  strlcpy(oledDocSrc, src, sizeof(oledDocSrc));
  return true;
}

// Picks the design for what is on show and draws it into oledBuf.
static bool oledRenderNow() {
  if (settings.oled == 0) return false;
  if (!oledScanned) oledScan();
  char src[APP_PATH_LEN + 4] = "";
  if (oledPreview[0] && millis() < oledPreviewUntil) {
    snprintf(src, sizeof(src), "f:%s", oledPreview);
  } else {
    oledPreview[0] = 0;
    char key[OLED_KEY_LEN];
    oledKey(key, sizeof(key));
    const char *path = oledFileFor(key);
    if (!path && !strncmp(key, "app:", 4)) path = oledFileFor("app");
    if (path) snprintf(src, sizeof(src), "f:%s", path);
    else {
      int b = oledBuiltinFor(key);
      if (b < 0) b = oledBuiltinFor("card");
      if (b >= 0) snprintf(src, sizeof(src), "b:%d", b);
    }
  }
  OledG g = { oledW(), oledH(), oledBuf };
  oledFrame++;
  if (!src[0] || !oledLoad(src)) {
    memset(oledBuf, 0, sizeof(oledBuf));
    return false;
  }
  oledRender(oledDoc->as<JsonVariantConst>(), oledField, g);
  return true;
}

// Called every loop while awake.
static void oledUpdate(uint32_t now) {
  if (settings.oled != oledKind && oled) {      // panel size changed in settings
    delete oled;
    oled = nullptr;
    oledForget();
  }
  if (!oled) {
    if (settings.oled && settings.oled != oledKind) { oledKind = settings.oled; oledBegin(); }
    if (!oled) return;
  }
  if (now - oledAt < OLED_MIN_MS) return;
  oledAt = now;
  oledRenderNow();
  size_t bytes = ((oledW() + 7) >> 3) * oledH();
  if (oledLastOk && !memcmp(oledBuf, oledLast, bytes)) return;
  memcpy(oledLast, oledBuf, bytes);
  oledLastOk = true;
  oled->setRotation(settings.oled_flip ? 2 : 0);
  oled->clearDisplay();
  oled->drawBitmap(0, 0, oledBuf, oledW(), oledH(), SSD1306_WHITE);
  oled->display();
}

// Watch Studio: show this file for `ms`, whatever is on screen.
static bool oledShowFile(const char *path, uint32_t ms) {
  if (!LittleFS.exists(path)) return false;
  strlcpy(oledPreview, path, sizeof(oledPreview));
  oledPreviewUntil = millis() + ms;
  oledDocSrc[0] = 0;                          // reload: the file may have just been rewritten
  oledAt = 0;
  return true;
}

static void oledBase64(const uint8_t *p, size_t n, String &out) {
  static const char T[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  out.reserve(out.length() + (n + 2) / 3 * 4);
  for (size_t i = 0; i < n; i += 3) {
    uint32_t v = (uint32_t)p[i] << 16 | (i + 1 < n ? p[i + 1] << 8 : 0) | (i + 2 < n ? p[i + 2] : 0);
    out += T[v >> 18 & 63];
    out += T[v >> 12 & 63];
    out += i + 1 < n ? T[v >> 6 & 63] : '=';
    out += i + 2 < n ? T[v & 63] : '=';
  }
}

// Before deep sleep: panel off, so it neither glows nor drains the battery.
static void oledOff() {
  if (!oled) return;
  oled->ssd1306_command(SSD1306_DISPLAYOFF);
}
