// Renders every case in cases.json with the watch's own renderer
// (firmware/watch_v1/oled_render.h) and prints one hex line per case.
// run.js makes the cases, runs this and compares with app/oled.js.
//
// Build (see .github/workflows/oled-parity.yml):
//   g++ -std=c++17 -I stub -I <ArduinoJson>/src -I <Adafruit GFX> -I ../../../firmware/watch_v1 parity.cpp
#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <ArduinoJson.h>
#include "oled_render.h"

static JsonObjectConst fields;

static bool field(const char *name, char *out, size_t n) {
  JsonVariantConst v = fields[name];
  if (!v.is<const char *>()) return false;
  snprintf(out, n, "%s", v.as<const char *>());
  return true;
}

int main(int argc, char **argv) {
  std::ifstream in(argc > 1 ? argv[1] : "cases.json", std::ios::binary);
  std::stringstream ss;
  ss << in.rdbuf();
  JsonDocument doc;
  if (deserializeJson(doc, ss.str())) { fprintf(stderr, "cannot read cases\n"); return 2; }
  static uint8_t buf[256 * 256 / 8];
  for (JsonObjectConst c : doc.as<JsonArrayConst>()) {
    fields = c["fields"];
    OledG g = { c["w"] | 128, c["h"] | 32, buf };
    oledRender(c["design"], field, g);
    size_t n = ((g.w + 7) >> 3) * g.h;
    for (size_t i = 0; i < n; i++) printf("%02x", buf[i]);
    printf("\n");
  }
  return 0;
}
