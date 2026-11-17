// Draws an OLED screen design (docs/OLED_SCREENS.md) into a 1-bit buffer.
//
// This is a line-by-line twin of app/oled.js (Watch Studio's preview): same
// shapes, same text fitting, same fonts. If one changes, change the other;
// tools/oled/parity/ checks that they agree pixel for pixel.
//
// Nothing here talks to the panel. The buffer is row-major, 8 pixels a byte, left
// pixel in the top bit - the format of Adafruit GFX's drawBitmap() and of the
// "bmp" item, so it can be drawn, compared and sent to Watch Studio as it is.
#pragma once
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Fonts/Org_01.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include "oled_assets.h"

// Font "f" numbers in a screen file. nullptr = the classic 5x7 font. Only add at the end.
static const GFXfont *const OLED_FONTS[] = {
  nullptr, &Org_01, &FreeSans9pt7b, &FreeSansBold9pt7b, &FreeSansBold12pt7b, &FreeSansBold18pt7b, &FreeSansBold24pt7b,
};
#define OLED_FONT_COUNT (int)(sizeof(OLED_FONTS) / sizeof(OLED_FONTS[0]))

#define OLED_DEG    0xB0       // the degree sign, after UTF-8 decoding
#define OLED_MAXC   160        // longest text, in characters
#define OLED_LINES  8          // most wrapped lines

// Looks up a field ("time", "batt", ...) for {name} in text. false = unknown field.
typedef bool (*OledFieldFn)(const char *name, char *out, size_t n);

struct OledG { int w, h; uint8_t *b; };

// ------------------------------------------------------------- numbers
static long oNum(JsonVariantConst v, long d) {
  double x;
  if (v.is<long>()) x = (double)v.as<long>();
  else if (v.is<double>()) x = v.as<double>();
  else return d;
  if (x != x) return d;                      // NaN
  if (x > 4096) x = 4096;
  if (x < -4096) x = -4096;
  return (long)x;                            // truncates toward zero, as Math.trunc
}
static bool oTruthy(JsonVariantConst v) { return v.is<bool>() ? v.as<bool>() : oNum(v, 0) != 0; }
static const char *oStr(JsonVariantConst v) { return v.is<const char *>() ? v.as<const char *>() : ""; }

// --------------------------------------------------------------- canvas
static void oPx(OledG &g, int x, int y, int c) {
  if (x < 0 || y < 0 || x >= g.w || y >= g.h) return;
  uint8_t &B = g.b[y * ((g.w + 7) >> 3) + (x >> 3)], m = 0x80 >> (x & 7);
  if (c) B |= m; else B &= ~m;
}
static void oFill(OledG &g, int x, int y, int w, int h, int c) {
  int i0 = x < 0 ? -x : 0, j0 = y < 0 ? -y : 0;
  if (w > g.w - x) w = g.w - x;              // the same pixels as the plain loops in oled.js, without the off-screen ones
  if (h > g.h - y) h = g.h - y;
  for (int j = j0; j < h; j++) for (int i = i0; i < w; i++) oPx(g, x + i, y + j, c);
}
static void oRect(OledG &g, int x, int y, int w, int h, int c) {
  if (w <= 0 || h <= 0) return;
  for (int i = 0; i < w; i++) { oPx(g, x + i, y, c); oPx(g, x + i, y + h - 1, c); }
  for (int j = 0; j < h; j++) { oPx(g, x, y + j, c); oPx(g, x + w - 1, y + j, c); }
}
// Bresenham, as Adafruit GFX's writeLine.
static void oLine(OledG &g, int x0, int y0, int x1, int y1, int c) {
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  int t;
  if (steep) { t = x0; x0 = y0; y0 = t; t = x1; x1 = y1; y1 = t; }
  if (x0 > x1) { t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
  int dx = x1 - x0, dy = abs(y1 - y0), ystep = y0 < y1 ? 1 : -1, err = dx / 2;
  for (; x0 <= x1; x0++) {
    if (steep) oPx(g, y0, x0, c); else oPx(g, x0, y0, c);
    err -= dy;
    if (err < 0) { y0 += ystep; err += dx; }
  }
}
static bool oInR(int dx, int dy, int r) { return r >= 0 && dx * dx + dy * dy <= r * r + r; }
static void oCircle(OledG &g, int cx, int cy, int r, bool fill, int c) {
  for (int dy = -r; dy <= r; dy++) {
    if (cy + dy < 0 || cy + dy >= g.h) continue;
    for (int dx = -r; dx <= r; dx++)
      if (cx + dx >= 0 && cx + dx < g.w && oInR(dx, dy, r) && (fill || !oInR(dx, dy, r - 1))) oPx(g, cx + dx, cy + dy, c);
  }
}
static bool oInRR(int i, int j, int w, int h, int r) {
  if (i < 0 || j < 0 || i >= w || j >= h) return false;
  int cx = i < r ? r : i > w - 1 - r ? w - 1 - r : i, cy = j < r ? r : j > h - 1 - r ? h - 1 - r : j;
  return oInR(i - cx, j - cy, r);
}
static void oRoundRect(OledG &g, int x, int y, int w, int h, int r, bool fill, int c) {
  if (w <= 0 || h <= 0) return;
  int m = (w < h ? w : h) >> 1;
  if (r > m) r = m;
  if (r < 0) r = 0;
  for (int j = 0; j < h; j++) {
    if (y + j < 0 || y + j >= g.h) continue;
    for (int i = 0; i < w; i++)
      if (x + i >= 0 && x + i < g.w && oInRR(i, j, w, h, r) && (fill || !oInRR(i - 1, j - 1, w - 2, h - 2, r - 1 < 0 ? 0 : r - 1)))
        oPx(g, x + i, y + j, c);
  }
}
static void oBitmap(OledG &g, int x, int y, const uint8_t *data, int w, int h, int c, int s) {
  int bw = (w + 7) >> 3;
  for (int j = 0; j < h; j++) for (int i = 0; i < w; i++)
    if (pgm_read_byte(&data[j * bw + (i >> 3)]) & (0x80 >> (i & 7))) {
      if (s == 2) oFill(g, x + i * 2, y + j * 2, 2, 2, c); else oPx(g, x + i, y + j, c);
    }
}

// ----------------------------------------------------------------- text
struct OledText { uint8_t c[OLED_MAXC + 2]; int n; };   // +2: room for the ".." of a cut line

// UTF-8 -> codes: printable ASCII, OLED_DEG for the degree sign, '?' for anything else.
static void oCodes(const char *s, OledText &t) {
  t.n = 0;
  const uint8_t *p = (const uint8_t *)s;
  while (*p && t.n < OLED_MAXC) {
    uint32_t cp = *p++;
    int more = cp >= 0xF0 ? 3 : cp >= 0xE0 ? 2 : cp >= 0xC0 ? 1 : 0;
    if (cp >= 0x80) cp &= more == 3 ? 0x07 : more == 2 ? 0x0F : 0x1F;
    while (more-- && (*p & 0xC0) == 0x80) cp = (cp << 6) | (*p++ & 0x3F);
    t.c[t.n++] = cp == 0xB0 ? OLED_DEG : (cp >= 32 && cp <= 126) ? (uint8_t)cp : '?';
  }
}
static const GFXglyph *oGlyph(const GFXfont *f, uint8_t c) { return &f->glyph[c - f->first]; }
static int oCap(const GFXfont *f) { return f ? -(int)oGlyph(f, 'H')->yOffset : 7; }
static int oDegR(const GFXfont *f) { int cap = oCap(f); return cap >= 24 ? 3 : cap >= 16 ? 2 : 1; }
static int oAdv(const GFXfont *f, uint8_t c) {
  if (!f) return 6;
  if (c == OLED_DEG) return 2 * oDegR(f) + 3;
  return c >= f->first && c <= f->last ? oGlyph(f, c)->xAdvance : 0;
}
static int oWidth(const GFXfont *f, const uint8_t *c, int n) { int w = 0; for (int i = 0; i < n; i++) w += oAdv(f, c[i]); return w; }
static void oDrawText(OledG &g, const GFXfont *f, const uint8_t *cs, int n, int x, int y, int col) {
  for (int k = 0; k < n; k++) {
    uint8_t c = cs[k];
    if (!f) {
      int ch = c == OLED_DEG ? 0xF8 : c;
      for (int i = 0; i < 5; i++) {
        uint8_t bits = pgm_read_byte(&OLED_CLASSIC_FONT[ch * 5 + i]);
        for (int j = 0; j < 8; j++, bits >>= 1) if (bits & 1) oPx(g, x + i, y - 7 + j, col);
      }
    } else if (c == OLED_DEG) {
      int r = oDegR(f);
      oCircle(g, x + r + 1, y - oCap(f) + r, r, false, col);
    } else if (c >= f->first && c <= f->last) {
      const GFXglyph *gl = oGlyph(f, c);
      const uint8_t *bm = f->bitmap + gl->bitmapOffset;
      int bit = 0;
      uint8_t bits = 0;
      for (int yy = 0; yy < gl->height; yy++) for (int xx = 0; xx < gl->width; xx++) {
        if (!(bit++ & 7)) bits = pgm_read_byte(bm++);
        if (bits & 0x80) oPx(g, x + gl->xOffset + xx, y + gl->yOffset + yy, col);
        bits <<= 1;
      }
    }
    x += oAdv(f, c);
  }
}
static void oEllipsize(const GFXfont *f, OledText &t, int mw) {
  static const uint8_t DOTS[2] = { '.', '.' };
  int dots = oWidth(f, DOTS, 2);
  while (t.n && oWidth(f, t.c, t.n) + dots > mw) t.n--;
  t.c[t.n++] = '.';
  t.c[t.n++] = '.';
}
// Splits into at most `lines` lines no wider than mw. Returns true if nothing was cut.
static bool oWrap(const GFXfont *f, const OledText &cs, int mw, int lines, OledText *out, int &count) {
  count = 0;
  int at = 0, end = cs.n;
  while (at < end && cs.c[at] == ' ') at++;                       // trimStart
  while (at < end && count < lines) {
    const uint8_t *rest = cs.c + at;
    int len = end - at;
    if (oWidth(f, rest, len) <= mw) { memcpy(out[count].c, rest, len); out[count++].n = len; at = end; break; }
    int n = 0, w = 0;
    while (n < len && w + oAdv(f, rest[n]) <= mw) { w += oAdv(f, rest[n]); n++; }
    if (n == 0) n = 1;
    int cut = n;
    if (rest[n] != ' ') {                                         // lastIndexOf(' ', n - 1)
      int sp = -1;
      for (int i = n - 1; i >= 0; i--) if (rest[i] == ' ') { sp = i; break; }
      if (sp > 0) cut = sp;
    }
    int k = cut;
    while (k > 0 && rest[k - 1] == ' ') k--;                      // trimEnd
    memcpy(out[count].c, rest, k);
    out[count++].n = k;
    at += cut;
    while (at < end && cs.c[at] == ' ') at++;                     // trimStart
  }
  bool fits = at >= end;
  if (!fits && count) oEllipsize(f, out[count - 1], mw);
  return fits;
}

// --------------------------------------------------------------- fields
// "{time} left" -> "10:25 left". Unknown fields stay as written, so a typo shows.
static void oExpand(const char *s, OledFieldFn field, char *out, size_t n) {
  size_t o = 0;
  char name[24], val[100];
  while (*s && o + 1 < n) {
    if (*s == '{') {
      const char *e = s + 1;
      while (isalnum((unsigned char)*e) || *e == '_') e++;
      size_t len = e - s - 1;
      if (*e == '}' && len > 0) {
        bool known = false;
        if (len < sizeof(name)) {
          memcpy(name, s + 1, len);
          name[len] = 0;
          val[0] = 0;
          known = field(name, val, sizeof(val));
        }
        if (known) {
          for (const char *v = val; *v && o + 1 < n; v++) out[o++] = *v;
          s = e + 1;
          continue;
        }
        for (const char *v = s; v <= e && o + 1 < n; v++) out[o++] = *v;   // left as written
        s = e + 1;
        continue;
      }
    }
    out[o++] = *s++;
  }
  out[o] = 0;
}
static bool oShown(JsonVariantConst item, OledFieldFn field) {
  const char *cond = oStr(item["if"]);
  if (!*cond) return true;
  bool neg = cond[0] == '!';
  char val[100] = "";
  bool known = field(neg ? cond + 1 : cond, val, sizeof(val));
  bool on = known && val[0] && strcmp(val, "0") != 0;
  return neg ? !on : on;
}

static const uint8_t *oIcon(const char *name, int size) {
  for (size_t i = 0; i < OLED_ICON_COUNT; i++)
    if (OLED_ICONS[i].size == size && !strcmp(OLED_ICONS[i].name, name)) return OLED_ICONS[i].data;
  return nullptr;
}
// Base64 -> bytes: skips anything that is not a base64 character and stops at '=', as oled.js.
static size_t oB64(const char *s, uint8_t *out, size_t max) {
  uint32_t acc = 0;
  int bits = 0;
  size_t n = 0;
  for (; *s && *s != '='; s++) {
    char ch = *s;
    int v = ch >= 'A' && ch <= 'Z' ? ch - 'A' : ch >= 'a' && ch <= 'z' ? ch - 'a' + 26 : ch >= '0' && ch <= '9' ? ch - '0' + 52
          : ch == '+' ? 62 : ch == '/' ? 63 : -1;
    if (v < 0) continue;
    acc = (acc << 6) | v;
    bits += 6;
    if (bits >= 8) { bits -= 8; if (n < max) out[n] = (acc >> bits) & 0xFF; n++; }
  }
  return n;
}
static int oPct(const char *tmpl, OledFieldFn field) {
  char s[100];
  oExpand(tmpl, field, s, sizeof(s));
  long p = atol(s);
  return p < 0 ? 0 : p > 100 ? 100 : (int)p;
}

// ---------------------------------------------------------------- items
static void oDrawItem(OledG &g, JsonVariantConst it, OledFieldFn field) {
  const char *k = oStr(it["k"]);
  int x = oNum(it["x"], 0), y = oNum(it["y"], 0), c = oNum(it["c"], 1) ? 1 : 0;
  if (!strcmp(k, "text")) {
    static char s[OLED_MAXC * 2];
    static OledText cs, lines[OLED_LINES];
    oExpand(oStr(it["s"]), field, s, sizeof(s));
    if (oTruthy(it["u"])) for (char *p = s; *p; p++) if (*p >= 'a' && *p <= 'z') *p -= 32;
    oCodes(s, cs);
    int fonts[8], nf = 0;
    if (it["f"].is<JsonArrayConst>()) {
      for (JsonVariantConst v : it["f"].as<JsonArrayConst>()) {
        long id = oNum(v, 0);
        if (id >= 0 && id < OLED_FONT_COUNT && nf < 8) fonts[nf++] = id;
      }
    } else {
      long id = oNum(it["f"], 0);
      if (id >= 0 && id < OLED_FONT_COUNT) fonts[nf++] = id;
    }
    if (!nf) fonts[nf++] = 0;
    int mw = oNum(it["mw"], g.w), want = oNum(it["wrap"], 1), count = 0;
    if (want < 1) want = 1;
    if (want > OLED_LINES) want = OLED_LINES;
    const char *a = oStr(it["a"]);
    const GFXfont *f = nullptr;
    for (int i = 0; i < nf; i++) {
      f = OLED_FONTS[fonts[i]];
      if (oWrap(f, cs, mw, want, lines, count)) break;
    }
    int lh = oNum(it["lh"], f ? f->yAdvance : 9);
    for (int i = 0; i < count; i++) {
      int w = oWidth(f, lines[i].c, lines[i].n);
      int x0 = !strcmp(a, "c") ? x - w / 2 : !strcmp(a, "r") ? x - w : x;
      oDrawText(g, f, lines[i].c, lines[i].n, x0, y + i * lh, c);
    }
  } else if (!strcmp(k, "icon")) {
    int z = oNum(it["z"], 16), s = z == 8 ? 8 : 16;
    char name[40];
    oExpand(oStr(it["n"]), field, name, sizeof(name));
    const uint8_t *data = oIcon(name, s);
    if (data) oBitmap(g, x, y, data, s, s, c, z == 32 ? 2 : 1);
  } else if (!strcmp(k, "battery")) {
    int w = oNum(it["w"], 20), h = oNum(it["h"], 9), p = oPct("{batt}", field), nub = h / 3;
    oRect(g, x, y, w - 2, h, c);
    oFill(g, x + w - 2, y + nub, 2, h - 2 * nub, c);
    oFill(g, x + 2, y + 2, (w - 6) * p / 100, h - 4, c);
  } else if (!strcmp(k, "bar")) {
    int w = oNum(it["w"], 40), h = oNum(it["h"], 6), r = oNum(it["r"], 0), p = oPct(oStr(it["v"]), field);
    if (h >= 5 && oNum(it["o"], 1)) {
      oRoundRect(g, x, y, w, h, r, false, c);
      oRoundRect(g, x + 2, y + 2, (w - 4) * p / 100, h - 4, r - 2 < 0 ? 0 : r - 2, true, c);
    } else {
      oRoundRect(g, x, y, w * p / 100, h, r, true, c);
    }
  } else if (!strcmp(k, "box")) {
    oRoundRect(g, x, y, oNum(it["w"], 10), oNum(it["h"], 10), oNum(it["r"], 0), oTruthy(it["fill"]), c);
  } else if (!strcmp(k, "line")) {
    oLine(g, x, y, oNum(it["x2"], x), oNum(it["y2"], y), c);
  } else if (!strcmp(k, "circle")) {
    oCircle(g, x, y, oNum(it["r"], 4), oTruthy(it["fill"]), c);
  } else if (!strcmp(k, "bmp")) {
    int w = oNum(it["w"], 0), h = oNum(it["h"], 0);
    if (w > 0 && h > 0 && w <= 256 && h <= 256) {
      size_t need = ((w + 7) >> 3) * h;
      uint8_t *data = (uint8_t *)malloc(need);
      if (data) {
        if (oB64(oStr(it["d"]), data, need) >= need) oBitmap(g, x, y, data, w, h, c, 1);
        free(data);
      }
    }
  }
}

// Clears the buffer and draws the design into it.
static void oledRender(JsonVariantConst design, OledFieldFn field, OledG &g) {
  memset(g.b, 0, ((g.w + 7) >> 3) * g.h);
  for (JsonVariantConst it : design["items"].as<JsonArrayConst>())
    if (it.is<JsonObjectConst>() && oShown(it, field)) oDrawItem(g, it, field);
  if (oTruthy(design["inv"])) {
    int bw = (g.w + 7) >> 3;
    for (int y = 0; y < g.h; y++) for (int x = 0; x < g.w; x++) g.b[y * bw + (x >> 3)] ^= 0x80 >> (x & 7);
  }
}
