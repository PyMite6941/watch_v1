"""Builds the Watch Studio installers to hand out with the watches.

    python installer/build.py

makes, from app/ and installer/:

    dist/WatchStudio-Windows.zip    Setup.cmd + files/   (unzip, double-click Setup.cmd)
    dist/WatchStudio-macOS.zip      Setup Watch Studio.command + files/

and (re)draws the icons it needs: app/icon.png and installer/windows/watch.ico,
from the same shapes as app/icon.svg. Plain Python 3, nothing to install.
Each zip is about 60 kB. Put them on a USB stick, a shared drive or a
download link next to the watch.
"""
import os
import stat
import struct
import sys
import zipfile
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = os.path.join(ROOT, "app")
INST = os.path.join(ROOT, "installer")
DIST = os.path.join(ROOT, "dist")
APP_FILES = ["index.html", "icon.svg", "icon.png", "manifest.webmanifest", "sw.js", "oled.js", "oled-assets.js"]

# ------------------------------------------------------------------ icon
# app/icon.svg, as shapes on a 96 x 96 grid: (kind, x, y, w, h, radius, colour).
STRAP, CASE, GLASS, SEG = (11, 110, 79), (29, 29, 31), (43, 43, 48), (255, 90, 79)
SHAPES = [("rect", 34, 2, 28, 92, 6, STRAP), ("rect", 12, 22, 72, 52, 10, CASE), ("rect", 18, 28, 60, 40, 5, GLASS)]
for x, y, w, h in [(25, 36, 9, 3), (25, 46.5, 9, 3), (25, 57, 9, 3), (24, 49, 3, 9), (32, 38, 3, 9),                   # "2"
                   (40, 36, 9, 3), (40, 57, 9, 3), (39, 38, 3, 20), (47, 38, 3, 20),                                   # "0"
                   (66, 38, 3, 20), (73, 38, 3, 20)]:                                                                  # "11"
    SHAPES.append(("rect", x, y, w, h, 0, SEG))
SHAPES += [("circle", 54, 43, 1.8, 0, 0, SEG), ("circle", 54, 53, 1.8, 0, 0, SEG)]


def inside(shape, px, py):
    kind, x, y, w, h, r, _ = shape
    if kind == "circle":
        return (px - x) ** 2 + (py - y) ** 2 <= w * w
    if not (x <= px <= x + w and y <= py <= y + h):
        return False
    # Rounded corners: outside the corner circle means outside.
    cx = min(max(px, x + r), x + w - r)
    cy = min(max(py, y + r), y + h - r)
    return (px - cx) ** 2 + (py - cy) ** 2 <= r * r + 1e-9


def render(size, ss=4):
    """RGBA rows at size x size, ss x ss samples per pixel."""
    scale = 96.0 / size
    rows = []
    for j in range(size):
        row = bytearray()
        for i in range(size):
            acc = [0, 0, 0, 0]
            for sj in range(ss):
                for si in range(ss):
                    px = (i + (si + 0.5) / ss) * scale
                    py = (j + (sj + 0.5) / ss) * scale
                    colour = None
                    for s in SHAPES:
                        if inside(s, px, py):
                            colour = s[6]
                    if colour:
                        acc[0] += colour[0]; acc[1] += colour[1]; acc[2] += colour[2]; acc[3] += 255
            n = ss * ss
            a = acc[3] / n
            if a:
                # Average colour of the covered samples, alpha = coverage.
                k = acc[3] / 255
                row += bytes([round(acc[0] / k), round(acc[1] / k), round(acc[2] / k), round(a)])
            else:
                row += b"\0\0\0\0"
        rows.append(bytes(row))
    return rows


def png(rows):
    size = len(rows)

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    raw = b"".join(b"\0" + r for r in rows)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def ico(images):
    """Windows icon holding PNG images (fine on Windows Vista and later)."""
    head = struct.pack("<HHH", 0, 1, len(images))
    offset = 6 + 16 * len(images)
    entries, blobs = b"", b""
    for size, data in images:
        entries += struct.pack("<BBBBHHII", size % 256, size % 256, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
        blobs += data
    return head + entries + blobs


def make_icons():
    print("drawing icons ...")
    big = png(render(256, 3))
    with open(os.path.join(APP, "icon.png"), "wb") as f:
        f.write(big)
    images = [(16, png(render(16))), (32, png(render(32))), (48, png(render(48))), (256, big)]
    with open(os.path.join(INST, "windows", "watch.ico"), "wb") as f:
        f.write(ico(images))


# ------------------------------------------------------------------ zips
def add(z, src, arc, executable=False):
    info = zipfile.ZipInfo(arc, date_time=(2026, 1, 1, 0, 0, 0))
    info.compress_type = zipfile.ZIP_DEFLATED
    mode = 0o755 if executable else 0o644
    info.external_attr = (stat.S_IFREG | mode) << 16      # keeps the Mac script runnable after unzipping
    with open(src, "rb") as f:
        data = f.read()
    if arc.endswith((".command", ".sh")):
        data = data.replace(b"\r\n", b"\n")                # a Mac shell script must not have Windows line ends
    if arc.endswith((".cmd", ".ps1")):
        data = data.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
    z.writestr(info, data)


def build():
    for f in ["index.html"]:
        if not os.path.exists(os.path.join(APP, f)):
            sys.exit("app/%s is missing" % f)
    os.makedirs(DIST, exist_ok=True)

    win = os.path.join(DIST, "WatchStudio-Windows.zip")
    with zipfile.ZipFile(win, "w") as z:
        top = "WatchStudio-Windows/"
        add(z, os.path.join(INST, "windows", "Setup.cmd"), top + "Setup.cmd")
        add(z, os.path.join(INST, "windows", "setup.ps1"), top + "files/setup.ps1")
        add(z, os.path.join(INST, "windows", "watch.ico"), top + "files/watch.ico")
        add(z, os.path.join(INST, "README-gift.txt"), top + "READ ME.txt")
        for f in APP_FILES:
            if os.path.exists(os.path.join(APP, f)):
                add(z, os.path.join(APP, f), top + "files/app/" + f)

    mac = os.path.join(DIST, "WatchStudio-macOS.zip")
    with zipfile.ZipFile(mac, "w") as z:
        top = "WatchStudio-macOS/"
        add(z, os.path.join(INST, "macos", "Setup Watch Studio.command"), top + "Setup Watch Studio.command", executable=True)
        add(z, os.path.join(INST, "README-gift.txt"), top + "READ ME.txt")
        for f in APP_FILES:
            if os.path.exists(os.path.join(APP, f)):
                add(z, os.path.join(APP, f), top + "files/app/" + f)

    for p in (win, mac):
        print("%-28s %6.1f kB" % (os.path.basename(p), os.path.getsize(p) / 1024))


if __name__ == "__main__":
    if "--no-icons" not in sys.argv:
        make_icons()
    build()
