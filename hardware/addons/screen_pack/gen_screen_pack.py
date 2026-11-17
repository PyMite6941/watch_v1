"""Generate the Screen Pack r2: a strip that holds a 0.91" OLED flat on top of the watch.

Must run with KiCad's bundled Python (it needs the pcbnew module):
    "%LOCALAPPDATA%\\Programs\\KiCad\\10.0\\bin\\python.exe" gen_screen_pack.py [--fab]

Writes kicad/screen_pack.kicad_pcb (placed AND routed by hand below), runs KiCad's
design-rule check, and with --fab writes fab/screen_pack_gerbers.zip: one zip with
copper, mask, silkscreen, drills and the outline on its own Edge_Cuts layer, which
is what JLCPCB, PCBWay and Elecrow ask for.

The strip lies flat on the lid, along the edge above the 7-segment display, and
plugs straight down into the watch's accessory socket J4 through the lid's opening.
The OLED module's own 4 header pins go through one of two columns of holes at the
left-hand end and are soldered underneath, so the screen lies flat over the strip,
pins on the left, glass facing up. The printed screen clip (enclosure.scad, part
"screen_clip") holds the screen down and snaps onto the case.

Seen from above (watch coordinates, mm; y grows towards the display):

  y=-5.3 ┌─A─B──────────────────────────────────────────────┐  overhangs the case edge (y=-2.5)
         │ ○ ○  DA DA                                         │  SDA
         │ ○ ○  CL CL                     J1 underneath:  ● ○ ○│  SCL
         │ ○ ○  V  G                      plastic sits in ○ ○ ○│  VCC / GND
         │ ○ ○  G  V                      the lid opening      │  GND / VCC
  y=5.3  └────────────────────────────────────────────────────┘  stops before the display window
        x=9.9                                                x=50.6

Column A is for modules labelled GND VCC SCL SDA, column B for VCC GND SCL SDA.
Solder the module into the ONE column whose letters match its own.

Where the pins are on the module comes from a seller's labelled photo
(protosupplies.com, "OLED-0.91-White-I2C-Connections.jpg"): display facing you, pins
on the left, top to bottom SDA SCL VCC GND, the row centred across the 12 mm width
and about 1.7 mm in from the left edge (read off the photo). Measure yours.

Every shared number (the strip outline, the module's size and position) is read from
enclosure/enclosure.scad, so the board and the clip cannot drift apart; check() fails
if they disagree.
"""
import os
import re
import shutil
import subprocess
import sys
import zipfile

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.join(HERE, "kicad")
FAB_DIR = os.path.join(HERE, "fab")
SCAD = os.path.join(HERE, "..", "..", "..", "enclosure", "enclosure.scad")
FP_DIR = os.environ.get(
    "KICAD_FOOTPRINT_DIR",
    os.path.expandvars(r"%LOCALAPPDATA%\Programs\KiCad\10.0\share\kicad\footprints"),
)
CLI = os.environ.get("KICAD_CLI") or shutil.which("kicad-cli") or os.path.expandvars(
    r"%LOCALAPPDATA%\Programs\KiCad\10.0\bin\kicad-cli.exe")

REV = "r2"
P = 2.54


def scad_values(path=SCAD):
    """The numbers this board shares with the case, read from enclosure.scad."""
    with open(path, encoding="utf-8") as f:
        text = f.read()
    out = {}
    for name in ("screen_strip", "oled", "oled_y", "oled_left", "oled_pin_inset", "accessory_center",
                 "accessory_opening", "display_center", "display_size", "window_margin", "wall", "gap",
                 "digit_h"):
        m = re.search(r"^" + name + r"\s*=\s*([^;]+);", text, re.M)
        if not m:
            raise SystemExit(f"enclosure.scad has no '{name} = ...;'")
        out[name] = eval(m.group(1).replace("[", "(").replace("]", ")"), {"__builtins__": {}})
    return out


S = scad_values()
SX0, SY0, SX1, SY1 = S["screen_strip"]       # strip outline in watch coordinates
W, H = SX1 - SX0, SY1 - SY0
OX, OY = 100.0, 100.0                        # where the board sits on KiCad's sheet


def local(x, y):
    """Watch coordinates -> this board's coordinates (origin = the strip's top-left corner)."""
    return (x - SX0, y - SY0)


# Watch socket J4 (hardware/scripts/gen_pcb.py: J4 at 43.2, 1.4, rotated 90).
WATCH_J4 = {"1": (43.2, 1.4), "2": (43.2, 3.94), "3": (45.74, 1.4), "4": (45.74, 3.94),
            "5": (48.28, 1.4), "6": (48.28, 3.94)}
J1_PIN1 = local(*WATCH_J4["1"])

# OLED columns: the module's pin row, top to bottom SDA SCL then the two power pins.
COL_A = S["oled_left"][0] + S["oled_pin_inset"]
COL_B = S["oled_left"][1] + S["oled_pin_inset"]
ROWS = [S["oled_y"] + (i - 1.5) * P for i in range(4)]     # SDA, SCL, power, power

PARTS = {
    "J1": ("Connector_PinHeader_2.54mm:PinHeader_2x03_P2.54mm_Vertical", "bottom",
           {"1": "GND", "2": "+3V3", "3": "SDA", "4": "SCL", "5": None, "6": None},
           ("1",) + J1_PIN1,
           lambda p: near(p["2"], (p["1"][0], p["1"][1] + P)) and near(p["3"], (p["1"][0] + P, p["1"][1]))),
    "J2": ("Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical", "top",
           {"1": "SDA", "2": "SCL", "3": "+3V3", "4": "GND"},
           ("1",) + local(COL_A, ROWS[0]),
           lambda p: near(p["2"], (p["1"][0], p["1"][1] + P))),
    "J3": ("Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical", "top",
           {"1": "SDA", "2": "SCL", "3": "GND", "4": "+3V3"},
           ("1",) + local(COL_B, ROWS[0]),
           lambda p: near(p["2"], (p["1"][0], p["1"][1] + P))),
}

# Tracks, in WATCH coordinates (converted below). Signals on top, power underneath.
SIG, PWR_W = 0.25, 0.4
J = WATCH_J4
JOG = J["1"][0] + P / 2                       # between J1's first two columns
BEND = COL_B + 1.26
_TRACKS = [   # (net, layer, width, [points]); "F" = top, "B" = bottom
    ("SDA", "F", SIG, [J["3"], (J["3"][0], ROWS[0]), (COL_A, ROWS[0])]),
    ("SCL", "F", SIG, [J["4"], (JOG, J["4"][1] - P / 2), (JOG, ROWS[1]), (COL_A, ROWS[1])]),
    ("GND", "B", PWR_W, [J["1"], (BEND, J["1"][1]), (COL_B, ROWS[2]), (COL_A, ROWS[3])]),
    ("+3V3", "B", PWR_W, [J["2"], (BEND, J["2"][1]), (COL_B, ROWS[3])]),
    ("+3V3", "F", PWR_W, [(COL_B, ROWS[3]), (COL_A, ROWS[2])]),
]
TRACKS = [(n, l, w, [local(*p) for p in pts]) for n, l, w, pts in _TRACKS]

_SILK = [  # text, x, y (watch coords), layer, size  (0.8 mm is the smallest the factory prints legibly)
    ("A", COL_B + 2.7, SY0 + 0.75, "F", 0.8), ("B", COL_B + 5.1, SY0 + 0.75, "F", 0.8),
    ("DA", COL_B + 2.7, ROWS[0], "F", 0.8), ("DA", COL_B + 5.1, ROWS[0], "F", 0.8),
    ("CL", COL_B + 2.7, ROWS[1], "F", 0.8), ("CL", COL_B + 5.1, ROWS[1], "F", 0.8),
    ("V", COL_B + 2.7, ROWS[2], "F", 0.8), ("G", COL_B + 5.1, ROWS[2], "F", 0.8),
    ("G", COL_B + 2.7, ROWS[3], "F", 0.8), ("V", COL_B + 5.1, ROWS[3], "F", 0.8),
    ("OLED PINS THIS END, SCREEN UP", 30.0, ROWS[1] + 1.27, "F", 0.8),
    ("1", J["1"][0] - 1.9, J["1"][1], "F", 0.8),
    ("1", J["1"][0] - 1.9, J["1"][1], "B", 0.8),
    ("SCREEN PACK " + REV, 28.0, ROWS[0], "B", 0.8),
    ("OLED: COLUMN A IF G V CL DA", 28.0, ROWS[1] + 0.2, "B", 0.8),
    ("COLUMN B IF V G CL DA", 28.0, ROWS[2] + 0.4, "B", 0.8),
]
SILK = [(t, *local(x, y), layer, size) for t, x, y, layer, size in _SILK]


def near(a, b, tol=0.02):
    return abs(a[0] - b[0]) < tol and abs(a[1] - b[1]) < tol


def mm(v):
    return pcbnew.FromMM(float(v))


def pt(x, y):
    return pcbnew.VECTOR2I(mm(OX + x), mm(OY + y))


def pads_mm(fp):
    out = {}
    for p in fp.Pads():
        out.setdefault(p.GetNumber(), (pcbnew.ToMM(p.GetPosition().x) - OX, pcbnew.ToMM(p.GetPosition().y) - OY))
    return out


def place(fp, side, anchor, test):
    if side == "bottom":
        fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
    for rot in (0, 90, 180, 270):
        fp.SetOrientationDegrees(rot)
        if test(pads_mm(fp)):
            break
    else:
        raise SystemExit(f"{fp.GetReference()}: no rotation puts its pins the right way round")
    num, x, y = anchor
    px, py = pads_mm(fp)[num]
    pos = fp.GetPosition()
    fp.SetPosition(pcbnew.VECTOR2I(pos.x + mm(x - px), pos.y + mm(y - py)))


def build():
    board = pcbnew.BOARD()
    ds = board.GetDesignSettings()
    ds.SetCopperLayerCount(2)
    nc = ds.m_NetSettings.GetDefaultNetclass()
    nc.SetClearance(mm(0.2))
    nc.SetTrackWidth(mm(SIG))
    nc.SetViaDiameter(mm(0.7))
    nc.SetViaDrill(mm(0.35))
    ds.m_CopperEdgeClearance = mm(0.3)

    nets = {}
    for _, _, pins, _, _ in PARTS.values():
        for n in pins.values():
            if n and n not in nets:
                nets[n] = pcbnew.NETINFO_ITEM(board, n)
                board.Add(nets[n])

    fps = {}
    for ref, (fpid, side, pins, anchor, test) in PARTS.items():
        lib, name = fpid.split(":")
        fp = pcbnew.FootprintLoad(os.path.join(FP_DIR, lib + ".pretty"), name)
        if fp is None:
            raise SystemExit(f"footprint not found: {fpid}")
        fp.SetFPID(pcbnew.LIB_ID(lib, name))
        fp.SetReference(ref)
        fp.Reference().SetVisible(False)
        board.Add(fp)
        place(fp, side, anchor, test)
        for pad in fp.Pads():
            n = pins.get(pad.GetNumber())
            if n:
                pad.SetNet(nets[n])
        fps[ref] = fp

    # Columns A and B are two places for the SAME module (only one is ever fitted), so
    # they overlap on purpose: J3 gives up its courtyard and J2's is widened over both.
    for item in list(fps["J3"].GraphicalItems()):
        if item.GetLayer() == pcbnew.F_CrtYd:
            fps["J3"].Remove(item)
    for item in fps["J2"].GraphicalItems():
        if item.GetLayer() == pcbnew.F_CrtYd and item.GetShape() == pcbnew.SHAPE_T_SEGMENT:
            for get, put in ((item.GetStart, item.SetStart), (item.GetEnd, item.SetEnd)):
                v = get()
                if v.x > fps["J2"].GetPosition().x:
                    put(pcbnew.VECTOR2I(v.x + mm(COL_B - COL_A), v.y))

    for a, b in (((0, 0), (W, 0)), ((W, 0), (W, H)), ((W, H), (0, H)), ((0, H), (0, 0))):
        s = pcbnew.PCB_SHAPE(board)
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetStart(pt(*a))
        s.SetEnd(pt(*b))
        s.SetLayer(pcbnew.Edge_Cuts)
        s.SetWidth(mm(0.1))
        board.Add(s)

    for net, layer, width, points in TRACKS:
        for (x1, y1), (x2, y2) in zip(points, points[1:]):
            t = pcbnew.PCB_TRACK(board)
            t.SetStart(pt(x1, y1))
            t.SetEnd(pt(x2, y2))
            t.SetWidth(mm(width))
            t.SetLayer(pcbnew.F_Cu if layer == "F" else pcbnew.B_Cu)
            t.SetNet(nets[net])
            board.Add(t)

    for text, x, y, layer, size in SILK:
        t = pcbnew.PCB_TEXT(board)
        t.SetText(text)
        t.SetPosition(pt(x, y))
        t.SetLayer(pcbnew.F_SilkS if layer == "F" else pcbnew.B_SilkS)
        t.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
        t.SetTextThickness(mm(0.12 if size >= 0.8 else 0.1))
        if layer == "B":
            t.SetMirrored(True)
        board.Add(t)
    return board, fps


def check(fps):
    """The board against the watch board and the case. Every line must say ok."""
    ok = True

    def expect(cond, msg):
        nonlocal ok
        print(("  ok   " if cond else "  FAIL ") + msg)
        ok = ok and cond

    def watch(p):
        return (p[0] + SX0, p[1] + SY0)

    j1 = {n: watch(p) for n, p in pads_mm(fps["J1"]).items()}
    expect(all(near(j1[n], WATCH_J4[n]) for n in WATCH_J4), "J1 pins sit exactly over the watch socket J4")
    expect(fps["J1"].IsFlipped(), "J1 header is underneath the board")

    # J1's plastic (7.62 x 5.08) must drop into the lid's accessory opening.
    cx, cy = (WATCH_J4["1"][0] + WATCH_J4["6"][0]) / 2, (WATCH_J4["1"][1] + WATCH_J4["6"][1]) / 2
    ox, oy = S["accessory_center"]
    ow, oh = S["accessory_opening"]
    expect(abs(cx - ox) + 3.81 <= ow / 2 - 0.2 and abs(cy - oy) + 2.54 <= oh / 2 - 0.2,
           "J1's plastic fits through the lid's accessory opening with 0.2 mm to spare")

    window_top = S["display_center"][1] - S["display_size"][1] / 2 - S["window_margin"]
    expect(SY1 <= window_top - 0.2 + 1e-6, f"strip stops {window_top - SY1:.1f} mm short of the display window")
    case_edge = -(S["gap"] + S["wall"])
    expect(SY0 < case_edge and SX0 > case_edge and SX1 < 58 + S["gap"] + S["wall"],
           f"strip overhangs only the top edge of the case, by {case_edge - SY0:.1f} mm")

    # The module, in both columns: inside the lid's length, clear of the digits.
    L, Wm, _ = S["oled"]
    digit_top = S["display_center"][1] - S["digit_h"] / 2
    for name, left in zip("AB", S["oled_left"]):
        expect(left >= case_edge and left + L <= 58 + S["gap"] + S["wall"],
               f"module in column {name} (x {left:.1f} to {left + L:.1f}) lies within the lid's length")
    expect(S["oled_y"] + Wm / 2 <= digit_top - 1.0,
           f"module's far edge stops {digit_top - S['oled_y'] - Wm / 2:.1f} mm short of the digits")
    expect(SY0 >= S["oled_y"] - Wm / 2 and SY1 <= S["oled_y"] + Wm / 2,
           "strip is no wider than the module, so it hides underneath")

    a, b = pads_mm(fps["J2"]), pads_mm(fps["J3"])
    expect(all(near(watch(a[str(i + 1)]), (COL_A, ROWS[i])) for i in range(4)), "column A: SDA SCL VCC GND from the top")
    expect(all(near(watch(b[str(i + 1)]), (COL_B, ROWS[i])) for i in range(4)), "column B: SDA SCL GND VCC from the top")
    for ref, fp in fps.items():
        xs = [p[0] for p in pads_mm(fp).values()]
        ys = [p[1] for p in pads_mm(fp).values()]
        expect(min(xs) > 1.0 and min(ys) > 1.0 and max(xs) < W - 1.0 and max(ys) < H - 0.6, f"{ref} pads inside the outline")
    return ok


def drc(path):
    os.makedirs(FAB_DIR, exist_ok=True)
    report = os.path.join(FAB_DIR, "screen_pack_drc.rpt")
    subprocess.run([CLI, "pcb", "drc", path, "-o", report, "--severity-all"], capture_output=True)
    problems = []
    with open(report, encoding="utf-8") as f:
        lines = f.read().splitlines()
    for i, line in enumerate(lines):
        if line.startswith("[") and not line.startswith(("[silk_", "[lib_footprint")):
            problems.append(line.strip() + "  " + (lines[i + 1].strip() if i + 1 < len(lines) else ""))
    print(f"design-rule check: {len(problems)} problem(s)")
    for p in problems[:12]:
        print("   ", p)
    return not problems


def fab(path):
    out = os.path.join(FAB_DIR, "gerbers")
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    layers = "F.Cu,B.Cu,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts"
    subprocess.run([CLI, "pcb", "export", "gerbers", path, "-o", out + os.sep, "--layers", layers,
                    "--subtract-soldermask"], check=True, capture_output=True)
    subprocess.run([CLI, "pcb", "export", "drill", path, "-o", out + os.sep, "--format", "excellon",
                    "--excellon-separate-th"], check=True, capture_output=True)
    archive = os.path.join(FAB_DIR, "screen_pack_gerbers.zip")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        for name in sorted(os.listdir(out)):
            z.write(os.path.join(out, name), name)
    print(f"ready to upload: fab/screen_pack_gerbers.zip ({W:.1f} x {H:.1f} mm)")


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    board, fps = build()
    good = check(fps)
    path = os.path.join(OUT_DIR, "screen_pack.kicad_pcb")
    pcbnew.SaveBoard(path, board)
    print("saved kicad/screen_pack.kicad_pcb")
    clean = drc(path)
    if "--fab" in sys.argv:
        if good and clean:
            fab(path)
        else:
            print("not exporting: fix the problems above first")
    if not (good and clean):
        raise SystemExit(1)


if __name__ == "__main__":
    try:
        main()
        code = 0
    except SystemExit as e:
        code = e.code if isinstance(e.code, int) else 1
        if not isinstance(e.code, int):
            print(e)
    sys.stdout.flush()
    os._exit(code)      # KiCad's Python can hang on a normal interpreter shutdown
