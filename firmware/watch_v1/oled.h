// Optional small OLED screen on the accessory socket (SSD1306, I2C address 0x3C or 0x3D).
//
// The four LED digits stay the main display. The OLED repeats the same screen in
// words: the full date, the whole weather ("Partly cloudy"), a message app's full
// text, all three sensor readings at once, the tuner's note and cents.
//
// settings.oled picks the panel: 0 off, 1 = 128x32 (0.91"), 2 = 128x64 (0.96"),
// 3 = 64x32 (0.49"). If nothing answers at 0x3C/0x3D it is simply skipped.
// Needs the Adafruit SSD1306 and Adafruit GFX libraries (docs/BUILD_GUIDE.md).
//
// Power: the panel is switched off (command 0xAE) before the watch
// sleeps, and only redrawn when the text changes, at most 4 times a second.
#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_MIN_MS 250

static Adafruit_SSD1306 *oled = nullptr;
static char     oledBig[24] = "", oledSmall[100] = "";
static uint32_t oledAt = 0;

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
  uint8_t w = settings.oled == 3 ? 64 : 128, h = settings.oled == 2 ? 64 : 32;
  oled = new Adafruit_SSD1306(w, h, &Wire, -1);
  // periphBegin = false: Wire is already running on the watch's pins.
  if (!oled->begin(SSD1306_SWITCHCAPVCC, addr, false, false)) { delete oled; oled = nullptr; return; }
  oled->cp437(true);                       // so '\xF8' is a degree sign
  oled->setTextWrap(false);
  oled->clearDisplay();
  oled->display();
  oledBig[0] = oledSmall[0] = 0;
}

// Largest text size (1-4) that fits `text` across the panel. Built-in font: 6 x 8 px per character at size 1.
static uint8_t oledFit(const char *text, uint8_t maxSize) {
  int n = strlen(text);
  for (uint8_t s = maxSize; s > 1; s--) if (n * 6 * s <= oled->width()) return s;
  return 1;
}

static void oledCentre(const char *text, uint8_t size, int y) {
  oled->setTextSize(size);
  int x = (oled->width() - (int)strlen(text) * 6 * size) / 2;
  oled->setCursor(x < 0 ? 0 : x, y);
  oled->print(text);
}

// Shows a big line (the value) and a small line (what it is), wrapped on tall panels.
static void oledShow(const char *big, const char *small) {
  if (!oled) return;
  if (!strcmp(big, oledBig) && !strcmp(small, oledSmall)) return;
  uint32_t now = millis();
  if (now - oledAt < OLED_MIN_MS) return;
  oledAt = now;
  strlcpy(oledBig, big, sizeof(oledBig));
  strlcpy(oledSmall, small, sizeof(oledSmall));

  oled->clearDisplay();
  oled->setTextColor(SSD1306_WHITE);
  bool tall = oled->height() >= 64;
  uint8_t size = oledFit(big, tall ? 4 : 2);
  oledCentre(big, size, tall ? 4 : (size == 2 ? 2 : 6));
  oled->setTextSize(1);
  int perLine = oled->width() / 6;
  int y = tall ? 40 : 24, lines = tall ? 3 : 1;
  int len = strlen(small);
  if (len <= perLine) oledCentre(small, 1, y);
  else {                                   // wrap at spaces
    const char *p = small;
    for (int l = 0; l < lines && *p; l++) {
      int take = strlen(p);
      if (take > perLine) {
        take = perLine;
        for (int i = perLine; i > perLine / 2; i--) if (p[i] == ' ') { take = i; break; }
      }
      char line[32];
      int n = take < (int)sizeof(line) - 1 ? take : (int)sizeof(line) - 1;
      memcpy(line, p, n);
      line[n] = 0;
      oled->setCursor(0, y + l * 9);
      oled->print(line);
      p += take;
      while (*p == ' ') p++;
    }
  }
  oled->display();
}

// Before deep sleep: panel off, so it neither glows nor drains the battery.
static void oledOff() {
  if (!oled) return;
  oled->ssd1306_command(SSD1306_DISPLAYOFF);
}
