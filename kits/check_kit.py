"""Check a kit's Watch Studio preset before loading it onto a watch.

    python kits/check_kit.py                     # every kits/*/preset.json
    python kits/check_kit.py my-kit/preset.json  # just one

A preset is an ordinary Watch Studio backup file holding only the settings the
kit cares about (screens, metronome speed...) and its custom apps. Load it with
Watch Studio > Backup > "Restore from a backup file...". Settings it does not
mention - name, time zone, Wi-Fi - stay as they are.

What is checked, because the watch would otherwise show blanks or refuse:
  - screens exist                          - at most 8 apps (MAX_APPS)
  - titles and step labels: 4 characters   - message text: 96 characters
  - every character has a seven-segment shape (no K, M, V, W, X or Z)
  - step lengths fit (1 s to 65535 s)      - countdown dates are YYYY-MM-DD
"""
import datetime
import glob
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

# firmware/watch_v1/settings.h VIEW_NAMES
VIEWS = {"time", "seconds", "date", "battery", "stopwatch", "timer", "counter", "dice", "period", "mesh", "weather",
         "temp", "humidity", "pressure", "tuner", "metronome"}
# firmware/watch_v1/watch_v1.ino glyph(): characters the display can draw
DRAWABLE = set("0123456789 -_*") | set("AaBbCcDdEeFfGgHhIiJjLlNnOoPpQqRrSsTtUuYy")
MAX_APPS, MAX_ITEMS, MAX_TEXT = 8, 32, 96


def check(path):
    errors = []

    def text_ok(where, s, limit):
        if not isinstance(s, str) or not s:
            errors.append(f"{where}: missing")
            return
        if len(s) > limit:
            errors.append(f"{where}: '{s}' is {len(s)} characters, the most is {limit}")
        bad = sorted({c for c in s if c not in DRAWABLE})
        if bad:
            errors.append(f"{where}: '{s}' uses {' '.join(bad)}, which the display cannot draw (shows a blank)")

    with open(path, encoding="utf-8") as f:
        b = json.load(f)
    if b.get("watch_backup") != 1 or not isinstance(b.get("cfg"), dict):
        errors.append('not a Watch Studio backup: needs "watch_backup": 1 and a "cfg" object')
        return errors
    cfg = b["cfg"]
    for v in cfg.get("views", []):
        if v not in VIEWS:
            errors.append(f"screen '{v}' does not exist (choose from {', '.join(sorted(VIEWS))})")
    for key in ("name", "tz", "wifi", "pin"):
        if key in cfg:
            errors.append(f"'{key}' is personal: leave it out of a kit so restoring does not overwrite it")
    if "hello" in cfg:
        text_ok("hello", cfg["hello"], 4)

    apps = {p: d for p, d in b.get("files", {}).items() if p.startswith("/apps/")}
    if len(apps) > MAX_APPS:
        errors.append(f"{len(apps)} apps; the watch loads at most {MAX_APPS}")
    for p, raw in sorted(apps.items()):
        if not p.endswith(".json"):
            errors.append(f"{p}: apps must be .json files")
            continue
        a = json.loads(raw) if isinstance(raw, str) else raw
        if not isinstance(raw, str):
            errors.append(f"{p}: file contents must be a JSON string, as Watch Studio writes them")
        kind = a.get("type")
        text_ok(f"{p} title", a.get("title"), 4)
        if kind == "sequence":
            steps = a.get("steps", [])
            if not steps or len(steps) > MAX_ITEMS:
                errors.append(f"{p}: needs 1-{MAX_ITEMS} steps")
            for i, s in enumerate(steps):
                text_ok(f"{p} step {i + 1}", s.get("label"), 4)
                if not (isinstance(s.get("s"), int) and 1 <= s["s"] <= 65535):
                    errors.append(f"{p} step {i + 1}: 's' must be whole seconds, 1-65535")
        elif kind == "picker":
            if "items" in a:
                if not 1 <= len(a["items"]) <= MAX_ITEMS:
                    errors.append(f"{p}: 1-{MAX_ITEMS} items")
                for i, it in enumerate(a["items"]):
                    text_ok(f"{p} item {i + 1}", it, 4)
            elif not (isinstance(a.get("max"), int) and a["max"] >= 2):
                errors.append(f"{p}: needs 'items' or a 'max' of 2 or more")
        elif kind == "countdown":
            try:
                datetime.date.fromisoformat(a.get("date", ""))
            except ValueError:
                errors.append(f"{p}: 'date' must be YYYY-MM-DD")
        elif kind == "message":
            text_ok(f"{p} text", a.get("text"), MAX_TEXT)
        else:
            errors.append(f"{p}: type must be sequence, picker, countdown or message")
    return errors


def main():
    paths = sys.argv[1:] or sorted(glob.glob(os.path.join(HERE, "*", "preset.json")))
    failed = 0
    for path in paths:
        errors = check(path)
        name = os.path.relpath(path)
        print(("ok    " if not errors else "FAIL  ") + name)
        for e in errors:
            print("        " + e)
        failed += bool(errors)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
