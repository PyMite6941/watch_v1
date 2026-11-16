"""Copy every kit's preset into Watch Studio, so the app can load a kit with one click.

    python kits/build_gallery.py           # rewrite the gallery inside app/index.html
    python kits/build_gallery.py --check   # only say whether the app's copy is current

The kits in kits/*/preset.json are the source; the app holds a copy between the
lines "/* KITS:BEGIN */" and "/* KITS:END */". Run this after adding or changing
a kit (check_kit.py reminds you). Plain Python 3.
"""
import glob
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
APP = os.path.join(HERE, "..", "app", "index.html")
BEGIN, END = "/* KITS:BEGIN */", "/* KITS:END */"
# Gallery order: the general one first, then by subject.
ORDER = ["everyday", "bees", "computer-science", "band", "calculus", "youth-group"]


def gallery():
    kits = []
    for path in glob.glob(os.path.join(HERE, "*", "preset.json")):
        with open(path, encoding="utf-8") as f:
            kits.append(json.load(f))
    kits.sort(key=lambda k: (ORDER.index(k["kit"]) if k["kit"] in ORDER else len(ORDER), k["kit"]))
    lines = ",\n".join("  " + json.dumps(k, ensure_ascii=False, separators=(",", ":")) for k in kits)
    return BEGIN + "\nconst KITS = [\n" + lines + "\n];\n" + END


def rebuilt():
    """(the app as it is, the app with a fresh gallery)"""
    with open(APP, encoding="utf-8") as f:
        html = f.read()
    a, b = html.find(BEGIN), html.find(END)
    if a < 0 or b < 0:
        sys.exit("app/index.html has no " + BEGIN + " ... " + END + " block")
    return html, html[:a] + gallery() + html[b + len(END):]


def current():
    html, new = rebuilt()
    return html == new


def main():
    html, new = rebuilt()
    if "--check" in sys.argv:
        if new != html:
            print("app/index.html's kit gallery is out of date: run python kits/build_gallery.py")
            return 1
        print("app/index.html's kit gallery is current")
        return 0
    with open(APP, "w", encoding="utf-8", newline="") as f:
        f.write(new)
    print("kit gallery written to app/index.html")
    return 0


if __name__ == "__main__":
    sys.exit(main())
