"""Generate the Screen Pack: a snap-on board that holds a small I2C OLED on the watch.

Must run with KiCad's bundled Python (it needs the pcbnew module):
    "%LOCALAPPDATA%\\Programs\\KiCad\\10.0\\bin\\python.exe" gen_screen_pack.py [--fab]

Writes kicad/screen_pack.kicad_pcb (placed AND routed by hand below), runs KiCad's
design-rule check, and with --fab writes fab/screen_pack_gerbers.zip: one zip with
copper, mask, silkscreen, drills and the outline on its own Edge_Cuts layer, which
is what JLCPCB, PCBWay and Elecrow ask for.

Same plug as the Pack Base (hardware/addons/pack_base): a 2x3 header underneath
that pushes into the watch's accessory socket J4. On top, two 4-hole rows for the
OLED module, one for each pin order cheap modules come in:

    y=0    ┌─────────────────────────┐   top edge: overhangs the watch
           │     G   V   CL  DA      │   row A: GND first (most 0.91" and 0.96" boards)
           │ A   ○   ○   ○   ○       │
           │     V   G   CL  DA      │   row B: VCC first
           │ B   ○   ○   ○   ○       │
           │      ● 1  ○ 3  ○ 5      │
    y=17.0 └──────○ 2──○ 4──○ 6──────┘   J1: 2x3 header underneath -> watch socket

Solder a 4-pin female header into the ONE row whose labels match the letters
printed on the OLED, so the screen plugs in and out. The other row stays empty.
Power is 3V3; the firmware switches the panel off (command 0xAE) before the watch
sleeps. If the screen ends up upside down, tick "Flip the OLED" in Watch Studio.
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

W, H = 20.0, 17.0
OX, OY = 100.0, 100.0
REV = "r1"

# The plug: exactly the Pack Base's J1, 1.3 mm from the edge nearest the display.
COL = (7.46, 10.0, 12.54)
ROW_BOT = H - 1.3
ROW_TOP = ROW_BOT - 2.54
ROW_A, ROW_B = 4.2, 8.0                      # the two OLED rows
OLED_X = (6.19, 8.73, 11.27, 13.81)

PARTS = {
    "J1": ("Connector_PinHeader_2.54mm:PinHeader_2x03_P2.54mm_Vertical", "bottom",
           {"1": "GND", "2": "+3V3", "3": "SDA", "4": "SCL", "5": None, "6": None},
           ("1", COL[0], ROW_TOP),
           lambda p: near(p["2"], (p["1"][0], p["1"][1] + 2.54)) and near(p["3"], (p["1"][0] + 2.54, p["1"][1]))),
    "J2": ("Connector_PinSocket_2.54mm:PinSocket_1x04_P2.54mm_Vertical", "top",
           {"1": "GND", "2": "+3V3", "3": "SCL", "4": "SDA"},
           ("1", OLED_X[0], ROW_A),
           lambda p: p["2"][0] > p["1"][0] and near((0, p["2"][1]), (0, p["1"][1]))),
    "J3": ("Connector_PinSocket_2.54mm:PinSocket_1x04_P2.54mm_Vertical", "top",
           {"1": "+3V3", "2": "GND", "3": "SCL", "4": "SDA"},
           ("1", OLED_X[0], ROW_B),
           lambda p: p["2"][0] > p["1"][0] and near((0, p["2"][1]), (0, p["1"][1]))),
}

SIG, PWR_W = 0.25, 0.4
TRACKS = [   # (net, layer, width, [points]); layer "F" = top, "B" = bottom
    ("SDA", "F", SIG, [(COL[1], ROW_TOP), (COL[1], 11.81), (OLED_X[3], ROW_B), (OLED_X[3], ROW_A)]),
    ("SCL", "B", SIG, [(COL[1], ROW_BOT), (OLED_X[2], ROW_BOT - 1.27), (OLED_X[2], ROW_B), (OLED_X[2], ROW_A)]),
    ("GND", "F", PWR_W, [(COL[0], ROW_TOP), (COL[0], 11.27), (OLED_X[1], 10.0), (OLED_X[1], ROW_B)]),
    ("GND", "F", PWR_W, [(OLED_X[1], ROW_B), (OLED_X[0], ROW_B - 2.54), (OLED_X[0], ROW_A)]),
    ("+3V3", "F", PWR_W, [(COL[0], ROW_BOT), (5.0, ROW_BOT), (5.0, ROW_B + 1.19), (OLED_X[0], ROW_B)]),
    ("+3V3", "B", PWR_W, [(OLED_X[0], ROW_B), (OLED_X[1], ROW_B - 2.54), (OLED_X[1], ROW_A)]),
]
VIAS = []

LABEL_A, LABEL_B = 1.9, 6.1
SILK = [  # text, x, y, layer, size  (0.8 mm is the smallest the factory prints legibly)
    ("G", OLED_X[0], LABEL_A, "F", 0.8), ("V", OLED_X[1], LABEL_A, "F", 0.8),
    ("CL", OLED_X[2], LABEL_A, "F", 0.8), ("DA", OLED_X[3], LABEL_A, "F", 0.8),
    ("V", OLED_X[0], LABEL_B, "F", 0.8), ("G", OLED_X[1], LABEL_B, "F", 0.8),
    ("CL", OLED_X[2], LABEL_B, "F", 0.8), ("DA", OLED_X[3], LABEL_B, "F", 0.8),
    ("A", 3.4, ROW_A, "F", 1.0), ("B", 3.4, ROW_B, "F", 1.0),
    ("1", 5.9, ROW_TOP, "F", 0.8),
    ("1", 5.0, ROW_TOP, "B", 0.8),
    ("SCREEN PACK " + REV, 10.0, 1.4, "B", 0.8),
    ("ONE ROW ONLY:", 10.0, 6.1, "B", 0.8),
    ("MATCH THE OLED", 10.0, 10.2, "B", 0.8),
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
    watch = {"1": (43.2, 1.4), "2": (43.2, 3.94), "3": (45.74, 1.4), "4": (45.74, 3.94), "5": (48.28, 1.4), "6": (48.28, 3.94)}
    dx, dy = 43.2 - COL[0], 1.4 - ROW_TOP
    expect(all(near((j1[n][0] + dx, j1[n][1] + dy), watch[n]) for n in watch), "J1 pins sit exactly over the watch socket J4")
    expect(fps["J1"].IsFlipped(), "J1 header is underneath the board")
    expect(3.94 + (H - ROW_BOT) < 5.7, "pack edge stops short of the display window")
    a, b = pads_mm(fps["J2"]), pads_mm(fps["J3"])
    expect([round(a[n][0], 2) for n in "1234"] == list(OLED_X) and [round(b[n][0], 2) for n in "1234"] == list(OLED_X),
           "both OLED rows run left to right, pin 1 on the left")
    for ref, fp in fps.items():
        xs = [p[0] for p in pads_mm(fp).values()]
        ys = [p[1] for p in pads_mm(fp).values()]
        expect(min(xs) > 0.8 and min(ys) > 0.8 and max(xs) < W - 0.8 and max(ys) < H - 0.8, f"{ref} pads inside the outline")
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
    print("ready to upload: fab/screen_pack_gerbers.zip")


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
