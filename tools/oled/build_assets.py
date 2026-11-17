#!/usr/bin/env python3
"""Builds the OLED screen pack's shared assets from one source, for both sides:

    firmware/watch_v1/oled_assets.h   icons + built-in screens, for the watch
    app/oled-assets.js                 the same icons and screens, plus the fonts,
                                       so Watch Studio's preview is pixel-exact

Sources:
    tools/oled/icons.txt      the icons, as text art ('#' lit, '.' dark)
    tools/oled/builtin.json   the screens the watch shows when nobody designed one
    tools/oled/starters.json  ready-made designs offered in Watch Studio (page only)
    the Adafruit GFX library  the fonts (the watch uses them straight from the
                              library; the page needs a copy). Found through
                              arduino-cli, or set ARDUINO_LIBS=<libraries folder>.

Run after changing any of them:  python tools/oled/build_assets.py
"""
import base64, json, os, re, shutil, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HERE = os.path.join(ROOT, "tools", "oled")
OUT_H = os.path.join(ROOT, "firmware", "watch_v1", "oled_assets.h")
OUT_JS = os.path.join(ROOT, "app", "oled-assets.js")

# id -> (header in the GFX library's Fonts/ folder or None for the built-in 5x7, name in Studio)
# The order is the "f" number in a screen file: never reorder, only add at the end.
FONTS = [
    (None, "Pixel 5x7"),
    ("Org_01", "Tiny caps"),
    ("FreeSans9pt7b", "Sans 9"),
    ("FreeSansBold9pt7b", "Sans bold 9"),
    ("FreeSansBold12pt7b", "Sans bold 12"),
    ("FreeSansBold18pt7b", "Sans bold 18"),
    ("FreeSansBold24pt7b", "Sans bold 24"),
]


def find_gfx():
    roots = []
    if os.environ.get("ARDUINO_LIBS"):
        roots.append(os.environ["ARDUINO_LIBS"])
    cli = shutil.which("arduino-cli")
    if cli:
        try:
            user = subprocess.run([cli, "config", "get", "directories.user"], capture_output=True, text=True, timeout=30).stdout.strip()
            if user:
                roots.append(os.path.join(user, "libraries"))
        except (OSError, subprocess.SubprocessError):
            pass
    home = os.path.expanduser("~")
    roots += [os.path.join(home, "Documents", "Arduino", "libraries"), os.path.join(home, "Arduino", "libraries")]
    for r in roots:
        p = os.path.join(r, "Adafruit_GFX_Library")
        if os.path.isfile(os.path.join(p, "Adafruit_GFX.h")):
            return p
    sys.exit("Adafruit GFX library not found. Install it (Library Manager, or arduino-cli lib install \"Adafruit GFX Library\") "
             "or set ARDUINO_LIBS to the folder that holds it.")


def c_bytes(text, name):
    """The byte list of `const uint8_t name[] PROGMEM = {...};` in a C file."""
    m = re.search(r"%s\s*\[\]\s*PROGMEM\s*=\s*\{(.*?)\};" % re.escape(name), text, re.S)
    if not m:
        sys.exit("could not read %s" % name)
    body = re.sub(r"//[^\n]*|/\*.*?\*/", "", m.group(1), flags=re.S)
    return bytes(int(v, 0) for v in re.findall(r"0x[0-9A-Fa-f]+|\b\d+\b", body))


def read_font(gfx, name):
    text = open(os.path.join(gfx, "Fonts", name + ".h"), encoding="utf-8", errors="replace").read()
    bitmap = c_bytes(text, name + "Bitmaps")
    g = re.search(r"%sGlyphs\s*\[\]\s*PROGMEM\s*=\s*\{(.*?)\};" % re.escape(name), text, re.S).group(1)
    glyphs = [[int(v, 0) for v in t] for t in re.findall(r"\{\s*(-?\w+),\s*(-?\w+),\s*(-?\w+),\s*(-?\w+),\s*(-?\w+),\s*(-?\w+)\s*\}", g)]
    first, last, y_adv = [int(v, 0) for v in re.search(
        r"GFXfont\s+%s\s+PROGMEM\s*=\s*\{.*?,.*?,\s*(\w+),\s*(\w+),\s*(\w+)\s*\}" % re.escape(name), text, re.S).groups()]
    assert len(glyphs) == last - first + 1, name
    return {"first": first, "last": last, "yAdvance": y_adv, "bitmap": base64.b64encode(bitmap).decode(),
            "glyphs": [v for gl in glyphs for v in gl]}


def read_icons():
    lines = open(os.path.join(HERE, "icons.txt"), encoding="utf-8").read().split("\n")
    icons, i = {}, 0
    while i < len(lines):
        line = lines[i].strip()
        if line.startswith("@"):
            name, size = line[1:].split()
            size = int(size)
            rows = [r.strip() for r in lines[i + 1:i + 1 + size]]
            if size not in (8, 16) or len(rows) != size or any(len(r) != size or set(r) - {"#", "."} for r in rows):
                sys.exit("icons.txt: %s must be %d rows of %d '#' or '.'" % (name, size, size))
            data = bytearray()
            for r in rows:
                for b in range(0, size, 8):
                    v = 0
                    for bit, ch in enumerate(r[b:b + 8].ljust(8, ".")):
                        if ch == "#":
                            v |= 0x80 >> bit
                    data.append(v)
            icons["%s@%d" % (name, size)] = bytes(data)
            i += 1 + size
        else:
            i += 1
    return icons


def main():
    gfx = find_gfx()
    version = re.search(r"version=(\S+)", open(os.path.join(gfx, "library.properties")).read()).group(1)
    classic = c_bytes(open(os.path.join(gfx, "glcdfont.c"), encoding="utf-8", errors="replace").read(), "font")
    assert len(classic) >= 1280, "glcdfont"
    fonts = [{"name": label, "classic": True, "bitmap": base64.b64encode(classic[:1280]).decode()} if f is None
             else dict(read_font(gfx, f), name=label) for f, label in FONTS]
    icons = read_icons()
    builtin = json.load(open(os.path.join(HERE, "builtin.json"), encoding="utf-8"))
    starters = json.load(open(os.path.join(HERE, "starters.json"), encoding="utf-8"))
    for size, designs in starters.items():
        if size.startswith("_"):
            continue
        w, h = (int(v) for v in size.split("x"))
        for d in designs:
            d.setdefault("v", 1)
            d.setdefault("w", w)
            d.setdefault("h", h)
    for size, screens in builtin.items():
        if size.startswith("_"):
            continue
        w, h = (int(v) for v in size.split("x"))
        for key, d in screens.items():
            d.setdefault("w", w)
            d.setdefault("h", h)
            d.setdefault("for", key)
            d.setdefault("v", 1)

    # ---- firmware header
    h = ["// Generated by tools/oled/build_assets.py from tools/oled/icons.txt and builtin.json. Do not edit.",
         "#pragma once", "#include <Arduino.h>", "",
         "struct OledIcon { const char *name; uint8_t size; const uint8_t *data; };", "",
         "// Adafruit GFX %s's classic 5x7 font (glcdfont.c, BSD licence, Adafruit Industries):" % version,
         "// 5 column bytes per character, bit 0 at the top. Copied because the library keeps it private.",
         "static const uint8_t OLED_CLASSIC_FONT[1280] PROGMEM = {"]
    for i in range(0, 1280, 20):
        h.append("  " + ", ".join("0x%02X" % b for b in classic[i:i + 20]) + ",")
    h += ["};", ""]
    for key, data in icons.items():
        h.append("static const uint8_t OLED_ICON_%s[] PROGMEM = { %s };" % (
            re.sub(r"\W", "_", key), ", ".join("0x%02X" % b for b in data)))
    h.append("static const OledIcon OLED_ICONS[] = {")
    for key in icons:
        name, size = key.split("@")
        h.append('  { "%s", %s, OLED_ICON_%s },' % (name, size, re.sub(r"\W", "_", key)))
    h += ["};", "#define OLED_ICON_COUNT (sizeof(OLED_ICONS) / sizeof(OLED_ICONS[0]))", "",
          "// Built-in screens: {panel width, height, screen key, JSON}. The watch uses",
          "// one of these whenever /oled has no file for that screen.",
          "struct OledBuiltin { uint8_t w, h; const char *key; const char *json; };",
          "static const OledBuiltin OLED_BUILTIN[] = {"]
    for size, screens in builtin.items():
        if size.startswith("_"):
            continue
        w, hh = size.split("x")
        for key, d in screens.items():
            h.append('  { %s, %s, "%s", R"OLED(%s)OLED" },' % (w, hh, key, json.dumps(d, separators=(",", ":"))))
    h += ["};", "#define OLED_BUILTIN_COUNT (sizeof(OLED_BUILTIN) / sizeof(OLED_BUILTIN[0]))", ""]
    with open(OUT_H, "w", newline="\n") as f:
        f.write("\n".join(h))

    # ---- page assets
    js = ["// Generated by tools/oled/build_assets.py. Do not edit.",
          "// Icons and built-in screens: from tools/oled/ (this project, MIT).",
          "// Fonts: copied from the Adafruit GFX library %s so the preview matches the watch:" % version,
          "//   'Pixel 5x7' is GFX's classic font (BSD licence, Adafruit Industries).",
          "//   'Tiny caps' is Org_v01 by Orgdot (www.orgdot.com/aliasfonts).",
          "//   The Sans fonts are GNU FreeFont (GPLv3 or later, with the font exception).",
          "window.OLED_ASSETS = " + json.dumps({
              "gfx": version,
              "fonts": fonts,
              "icons": {k: base64.b64encode(v).decode() for k, v in icons.items()},
              "builtin": {k: v for k, v in builtin.items() if not k.startswith("_")},
              "starters": {k: v for k, v in starters.items() if not k.startswith("_")},
          }, separators=(",", ":")) + ";", ""]
    with open(OUT_JS, "w", newline="\n") as f:
        f.write("\n".join(js))
    print("GFX %s: %d fonts, %d icons, %d built-in screens -> %s, %s (%d kB)" % (
        version, len(fonts), len(icons), sum(len(v) for k, v in builtin.items() if not k.startswith("_")),
        os.path.relpath(OUT_H, ROOT), os.path.relpath(OUT_JS, ROOT), os.path.getsize(OUT_JS) // 1024))


if __name__ == "__main__":
    main()
