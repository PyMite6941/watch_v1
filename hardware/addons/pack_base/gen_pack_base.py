"""Generate the Pack Base: one small board that every add-on pack is built on.

Must run with KiCad's bundled Python (it needs the pcbnew module):
    "%LOCALAPPDATA%\\Programs\\KiCad\\10.0\\bin\\python.exe" gen_pack_base.py [--fab]

Writes kicad/pack_base.kicad_pcb (placed AND routed - the board is small enough
that the tracks are listed by hand below), runs KiCad's design-rule check, and
with --fab also writes fab/pack_base_gerbers.zip for the PCB factory.

Coordinates are millimetres from the board's top-left corner, looking at the
TOP side, with the pack plugged in: the bottom edge of this board is the edge
nearest the watch's display, the top edge sticks out past the watch.

    y=0    ┌──────── [Qwiic] ────────┐   top edge: overhangs the watch
           │  V  G SCL SDA  -  -     │   J2: 6-hole row for a breakout board
           │ [JP1]             ○ G   │   JP1: sensor power, 3V3 (always) or AUX (awake only)
           │                   ○ AUX │   J3: 3-hole row for a microphone board
           │      ● 1  ○ 3  ○ 5     │
    y=18.6 └──────○ 2──○ 4──○ 6─○AIN┘   J1: 2x3 header underneath -> watch socket
"""
import os
import shutil
import subprocess
import sys
import zipfile

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.join(HERE, "kicad")
FAB_DIR = os.path.join(HERE, "fab")
FP_DIR = os.environ.get(
    "KICAD_FOOTPRINT_DIR",
    os.path.expandvars(r"%LOCALAPPDATA%\Programs\KiCad\10.0\share\kicad\footprints"),
)
CLI = os.environ.get("KICAD_CLI") or shutil.which("kicad-cli") or os.path.expandvars(
    r"%LOCALAPPDATA%\Programs\KiCad\10.0\bin\kicad-cli.exe")

W, H = 20.0, 18.6
OX, OY = 100.0, 100.0
REV = "r1"

# Watch socket J4, seen from the front: top row 1 GND, 3 SDA, 5 AUX; bottom row 2 3V3, 4 SCL, 6 AIN.
# The pack's header sits underneath at exactly those positions, so the pins line up with
# hardware/kicad (J4 pads at x 43.2 / 45.74 / 48.28, y 1.4 / 3.94).
COL = (7.46, 10.0, 12.54)
ROW_TOP, ROW_BOT = 14.76, 17.30           # lower row 1.3 mm from the edge: clears the display window
J2Y = 8.2                                 # breakout row, clear of the Qwiic socket's courtyard

# ref: footprint, side, {pad: net}, (pad that is anchored, x, y), orientation test
PARTS = {
    "J1": ("Connector_PinHeader_2.54mm:PinHeader_2x03_P2.54mm_Vertical", "bottom",
           {"1": "GND", "2": "+3V3", "3": "SDA", "4": "SCL", "5": "AUX", "6": "AIN"},
           ("1", COL[0], ROW_TOP),
           lambda p: near(p["2"], (p["1"][0], p["1"][1] + 2.54)) and near(p["3"], (p["1"][0] + 2.54, p["1"][1]))),
    # Breakout row, in the order most cheap I2C boards use: GY-BME280 / BMP280, BH1750, GY-521, AHT20...
    "J2": ("Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical", "top",
           {"1": "PWR", "2": "GND", "3": "SCL", "4": "SDA", "5": None, "6": None},
           ("1", 3.65, J2Y),
           lambda p: p["2"][0] > p["1"][0] and near((0, p["2"][1]), (0, p["1"][1]))),
    # Microphone row: MAX4466 / MAX9814 board. VCC goes to AUX so the mic is off while the watch sleeps.
    "J3": ("Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical", "top",
           {"1": "AIN", "2": "AUX", "3": "GND"},
           ("1", 16.5, ROW_BOT),
           lambda p: p["2"][1] < p["1"][1] and near((p["2"][0], 0), (p["1"][0], 0))),
    # Qwiic / STEMMA QT socket (JST SH 4-pin): 1 GND, 2 3V3, 3 SDA, 4 SCL. Cable leaves from the top edge.
    "J5": ("Connector_JST:JST_SH_SM04B-SRSS-TB_1x04-1MP_P1.00mm_Horizontal", "top",
           {"1": "GND", "2": "PWR", "3": "SDA", "4": "SCL", "MP": None},
           ("4", 8.5, 5.1),
           lambda p: p["1"][0] > p["4"][0] and p["MP"][1] < p["1"][1]),
    # Sensor power: pads 1-2 bridged at the factory = always on (3V3). Cut the bridge and
    # blob 2-3 for AUX = on only while the watch is awake (modules with a power LED, MPU6050...).
    "JP1": ("Jumper:SolderJumper-3_P1.3mm_Bridged12_RoundedPad1.0x1.5mm", "top",
            {"1": "+3V3", "2": "PWR", "3": "AUX"},
            ("1", 2.2, 11.6),
            lambda p: p["3"][0] > p["1"][0]),
}

SIG, PWR_W = 0.25, 0.4
# (net, layer, width, [points])  layer "F" = top, "B" = bottom
TRACKS = [
    # header -> microphone row (top)
    ("AIN", "F", SIG, [(COL[2], ROW_BOT), (16.5, ROW_BOT)]),
    ("AUX", "F", SIG, [(COL[2], ROW_TOP), (16.5, ROW_TOP)]),
    # AUX -> jumper pad 3 (top), above the header's top row
    ("AUX", "F", SIG, [(COL[2], ROW_TOP), (11.6, 13.0), (5.5, 13.0), (3.5 + 1.3, 11.6)]),
    # 3V3 -> jumper pad 1 (top), round the left of the header
    ("+3V3", "F", PWR_W, [(COL[0], ROW_BOT), (2.2, ROW_BOT), (2.2, 11.6)]),
    # jumper pad 2 -> breakout VCC (top)
    ("PWR", "F", PWR_W, [(3.5, 11.6), (3.65, J2Y)]),
    # GND: mic row -> breakout GND (top); header GND -> breakout GND (bottom)
    ("GND", "F", PWR_W, [(16.5, 12.22), (15.6, 11.3), (6.19, 11.3), (6.19, J2Y)]),
    ("GND", "B", PWR_W, [(COL[0], ROW_TOP), (7.46, 9.6), (6.19, J2Y)]),
    # I2C from the header up to the breakout row (bottom)
    ("SDA", "B", SIG, [(COL[1], ROW_TOP), (11.27, 13.49), (11.27, J2Y)]),
    ("SCL", "B", SIG, [(COL[1], ROW_BOT), (8.73, 16.03), (8.73, J2Y)]),
    # breakout row -> Qwiic (top for the data lines, bottom for power through two vias)
    ("SCL", "F", SIG, [(8.73, J2Y), (8.5, 6.4), (8.5, 5.1)]),
    ("SDA", "F", SIG, [(11.27, J2Y), (11.27, 6.4), (9.5, 6.4), (9.5, 5.1)]),
    ("PWR", "F", PWR_W, [(10.5, 5.1), (10.5, 2.5)]),
    ("GND", "F", PWR_W, [(11.5, 5.1), (11.5, 3.5)]),
    ("PWR", "B", PWR_W, [(10.5, 2.5), (3.65, 2.5), (3.65, J2Y)]),
    ("GND", "B", PWR_W, [(11.5, 3.5), (6.19, 3.5), (6.19, J2Y)]),
]
VIAS = [("PWR", 10.5, 2.5), ("GND", 11.5, 3.5)]

SILK = [  # text, x, y, layer, size  (0.8 mm is the smallest the factory prints legibly)
    ("1", 5.9, ROW_TOP, "F", 0.8),
    ("V", 3.65, 9.75, "F", 0.8), ("G", 6.19, 9.75, "F", 0.8), ("CL", 8.73, 9.75, "F", 0.8),
    ("DA", 11.27, 9.75, "F", 0.8), ("-", 13.81, 9.75, "F", 0.8), ("-", 16.35, 9.75, "F", 0.8),
    ("G", 18.6, 12.22, "F", 0.8), ("X", 18.6, ROW_TOP, "F", 0.8), ("A", 18.6, ROW_BOT, "F", 0.8),
    ("3", 2.2, 13.4, "F", 0.8), ("X", 4.8, 13.4, "F", 0.8),
    ("1", 5.9, ROW_TOP, "B", 0.8),
    ("PACK BASE " + REV, 10.0, 1.4, "B", 0.8),
    ("AWAY FROM DISPLAY", 10.0, 5.6, "B", 0.8),
    ("- - SDA SCL G V", 10.0, 9.75, "B", 0.8),        # seen from underneath the row runs right to left
    ("PIN 1 = LID NOTCH", 10.0, 12.6, "B", 0.8),
    ("G", 18.6, 12.22, "B", 0.8), ("X", 18.6, ROW_TOP, "B", 0.8), ("A", 18.6, ROW_BOT, "B", 0.8),
]


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
    ds.m_CopperEdgeClearance = mm(0.3)      # what JLCPCB / PCBWay accept; same as the watch board

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
    for net, x, y in VIAS:
        v = pcbnew.PCB_VIA(board)
        v.SetViaType(pcbnew.VIATYPE_THROUGH)
        v.SetPosition(pt(x, y))
        v.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
        try:
            v.SetWidth(pcbnew.F_Cu, mm(0.7))
        except TypeError:
            v.SetWidth(mm(0.7))
        v.SetDrill(mm(0.35))
        v.SetNet(nets[net])
        board.Add(v)

    for text, x, y, layer, size in SILK:
        t = pcbnew.PCB_TEXT(board)
        t.SetText(text)
        t.SetPosition(pt(x, y))
        t.SetLayer(pcbnew.F_SilkS if layer == "F" else pcbnew.B_SilkS)
        t.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
        t.SetTextThickness(mm(0.12))
        if layer == "B":
            t.SetMirrored(True)
        board.Add(t)
    return board, fps


def check(fps):
    ok = True

    def expect(cond, msg):
        nonlocal ok
        print(("  ok   " if cond else "  FAIL ") + msg)
        ok = ok and cond

    j1 = pads_mm(fps["J1"])
    # The same pin positions as the watch's socket, shifted by (43.2 - 7.46, 1.4 - 14.16).
    watch = {"1": (43.2, 1.4), "2": (43.2, 3.94), "3": (45.74, 1.4), "4": (45.74, 3.94), "5": (48.28, 1.4), "6": (48.28, 3.94)}
    dx, dy = 43.2 - COL[0], 1.4 - ROW_TOP
    expect(all(near((j1[n][0] + dx, j1[n][1] + dy), watch[n]) for n in watch), "J1 pins sit exactly over the watch socket J4")
    expect(fps["J1"].IsFlipped(), "J1 header is underneath the board")
    expect(H - ROW_BOT <= 1.5, "board ends within 1.5 mm of the lower pin row (clears the display window)")
    # Lower pin row is 3.94 mm into the watch board; the display window begins at 5.7 mm.
    expect(3.94 + (H - ROW_BOT) < 5.7, "pack edge stops short of the display window")
    for ref, fp in fps.items():
        xs = [p[0] for p in pads_mm(fp).values()]
        ys = [p[1] for p in pads_mm(fp).values()]
        expect(min(xs) > 0.8 and min(ys) > 0.8 and max(xs) < W - 0.8 and max(ys) < H - 0.8, f"{ref} pads inside the outline")
    return ok


def drc(path):
    os.makedirs(FAB_DIR, exist_ok=True)
    report = os.path.join(FAB_DIR, "pack_base_drc.rpt")
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
    archive = os.path.join(FAB_DIR, "pack_base_gerbers.zip")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        for name in sorted(os.listdir(out)):
            z.write(os.path.join(out, name), name)
    print("ready to upload: fab/pack_base_gerbers.zip")


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    board, fps = build()
    good = check(fps)
    path = os.path.join(OUT_DIR, "pack_base.kicad_pcb")
    pcbnew.SaveBoard(path, board)
    print("saved kicad/pack_base.kicad_pcb")
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
