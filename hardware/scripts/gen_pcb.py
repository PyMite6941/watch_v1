"""Generate hardware/kicad/watch_v1.kicad_pcb from design.py.

Must run with KiCad's bundled Python (it needs the pcbnew module):
    "%LOCALAPPDATA%\\Programs\\KiCad\\10.0\\bin\\python.exe" gen_pcb.py [--out DIR]

It places every footprint, assigns nets and draws the outline, and writes
watch_v1.pads.json. Routing is a separate step (route.py, any Python 3), so the
full build is:

    kicad-python gen_pcb.py     # place, export pad geometry
    python route.py             # autoroute -> watch_v1.routes.json
    kicad-python gen_pcb.py     # place again, this time with the tracks

Routes are only applied when they were made for the current placement.

Board coordinates below are millimetres from the board's top-left corner,
looking at the TOP (display) side.
"""
import hashlib
import json
import os
import sys

import pcbnew

from design import PARTS, SUPERMINI_PINS, nets
from gen_sch import uid

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.abspath(os.path.join(HERE, "..", "kicad"))
if "--out" in sys.argv:
    OUT_DIR = os.path.abspath(sys.argv[sys.argv.index("--out") + 1])
FP_DIR = os.environ.get(
    "KICAD_FOOTPRINT_DIR",
    os.path.expandvars(r"%LOCALAPPDATA%\Programs\KiCad\10.0\share\kicad\footprints"),
)

BOARD_W, BOARD_H = 58.0, 44.0
OX, OY = 100.0, 100.0           # where the board sits on the KiCad page
HOLES = [(3, 3), (55, 3), (3, 41), (55, 41)]
POWER_NETS = ["GND", "+3V3", "VBUS", "VBAT", "MOTOR_N"]

TOP, BOT = "top", "bottom"
# ref: (side, rotation, x, y) where x, y is the top-left corner of the box around the pad centres
PLACE = {
    "DS1": (TOP, 90, 22.65, 7.88),
    "U1": (BOT, None, 37.86, 8.0),        # rotation chosen so the USB-C end faces the right edge
    "U2": (BOT, 90, 2.2, 7.0),
    "U3": (BOT, 90, 2.2, 17.0),
    "J1": (BOT, 0, 24.0, 10.42),
    "J4": (TOP, 90, 43.2, 1.4),           # accessory socket, reached through the lid
    "C4": (TOP, 90, 51.4, 1.8),
    "C1": (TOP, 0, 7.5, 3.0),
    # --- strip below the display, top side (row A: sensors, LED, small parts)
    "C2": (TOP, 0, 3.0, 27.0),
    "C3": (TOP, 0, 8.0, 27.0),
    "R21": (TOP, 0, 12.8, 27.7),
    "R20": (TOP, 270, 19.0, 26.8),
    "R17": (TOP, 270, 22.1, 26.8),
    "D3": (TOP, 0, 27.095, 28.5),
    "R14": (TOP, 270, 33.8, 26.8),
    "R15": (TOP, 270, 36.9, 26.8),
    "R16": (TOP, 270, 40.0, 26.8),
    "D1": (TOP, 0, 43.2, 26.9),
    "SW1": (TOP, 0, 4.5, 32.4),
    "SW2": (TOP, 0, 47.0, 32.4),
    "R18": (TOP, 270, 40.4, 33.3),
    "R19": (TOP, 270, 43.5, 33.3),
    "D2": (TOP, 0, 41.3, 39.5),
    "J2": (BOT, 90, 43.2, 42.3),
    "J3": (BOT, 90, 7.0, 42.0),
}
for i in range(8):
    PLACE[f"R{i + 1}"] = (TOP, 270, 18.15 + 3.1 * i, 1.8)
for i in range(5):
    PLACE[f"Q{i + 1}"] = (TOP, 0, 14.0 + 5.5 * i, 39.7)
    PLACE[f"R{9 + i}"] = (TOP, 270, 15.27 + 5.5 * i, 33.3)
for i, (hx, hy) in enumerate(HOLES):
    PLACE[f"H{i + 1}"] = (TOP, 0, hx, hy)

SILK = [  # (text, x, y, side, size)
    ("WATCH V1 r5", 29, 43.0, TOP, 0.8),
    ("MODE", 7.75, 39.6, TOP, 0.9),
    ("SET", 50.25, 39.6, TOP, 0.9),
    ("G BAT CHG", 45.74, 40.3, BOT, 0.8),
    ("MOTOR + -", 8.3, 40.0, BOT, 0.8),
    ("RTC", 27.3, 9.4, BOT, 0.8),
    ("3V3", 27.3, 10.42, BOT, 0.8), ("SDA", 27.3, 12.96, BOT, 0.8), ("SCL", 27.3, 15.5, BOT, 0.8),
    ("NC", 27.3, 18.04, BOT, 0.8), ("GND", 27.3, 20.58, BOT, 0.8),
    ("ACCESSORY", 45.74, 6.2, BOT, 0.8),
    ("USB-C THIS END >", 46.8, 15.6, BOT, 0.9),
    ("BATTERY SIDE", 29, 36.5, BOT, 1.0),
]


def mm(v):
    return pcbnew.FromMM(float(v))


def pt(x, y):
    return pcbnew.VECTOR2I(mm(OX + x), mm(OY + y))


# ------------------------------------------------------- project footprint lib
def write_supermini_footprint():
    lib = os.path.join(OUT_DIR, "watch_v1.pretty")
    os.makedirs(lib, exist_ok=True)
    w, length = 18.0, 22.52
    hx, hy = w / 2, length / 2
    out = []
    out.append('(footprint "ESP32-C3_SuperMini"\n\t(version 20260206)\n\t(generator "watch_v1_gen")\n\t(layer "F.Cu")')
    out.append('\t(descr "ESP32-C3 SuperMini module, 2x8 pins, 2.54 mm pitch, 15.24 mm row spacing. Drawn from the module top, USB-C at the top.")')

    def text(kind, name, value, x, y, layer, size=1.0, rot=0, hide=False):
        head = f'(property "{name}" "{value}"' if kind == "property" else f'(fp_text user "{value}"'
        out.append(f'\t{head}\n\t\t(at {x} {y} {rot})\n\t\t(layer "{layer}")' + ("\n\t\t(hide yes)" if hide else "") +
                   f'\n\t\t(effects\n\t\t\t(font\n\t\t\t\t(size {size} {size})\n\t\t\t\t(thickness {0.15 if size >= 1 else 0.12})\n\t\t\t)\n\t\t)\n\t)')

    text("property", "Reference", "REF**", 0, -hy - 1.2, "F.SilkS")
    text("property", "Value", "ESP32-C3_SuperMini", 0, hy + 1.2, "F.Fab")
    out.append("\t(attr through_hole)")

    def rect(x1, y1, x2, y2, layer, width):
        out.append(f'\t(fp_rect\n\t\t(start {x1} {y1})\n\t\t(end {x2} {y2})\n\t\t(stroke\n\t\t\t(width {width})\n\t\t\t(type solid)\n\t\t)\n\t\t(fill no)\n\t\t(layer "{layer}")\n\t)')

    rect(-hx, -hy, hx, hy, "F.SilkS", 0.12)
    rect(-hx, -hy, hx, hy, "F.Fab", 0.1)
    rect(-4.5, -hy - 1.0, 4.5, -hy + 6.3, "F.SilkS", 0.12)       # USB-C connector
    rect(-hx - 0.25, -hy - 1.25, hx + 0.25, hy + 0.25, "F.CrtYd", 0.05)
    text("fp_text", "", "USB", 0, -hy + 3.0, "F.SilkS", 0.8)
    for n in range(1, 17):
        left = n <= 8
        row = n - 1 if left else 16 - n
        x = -7.62 if left else 7.62
        y = -8.89 + row * 2.54
        shape = "rect" if n == 1 else "circle"
        out.append(f'\t(pad "{n}" thru_hole {shape}\n\t\t(at {x} {y})\n\t\t(size 1.7 1.7)\n\t\t(drill 1)\n\t\t(layers "*.Cu" "*.Mask")\n\t\t(remove_unused_layers no)\n\t)')
        label = SUPERMINI_PINS[str(n)].replace("GPIO", "")
        text("fp_text", "", label, (x + 2.3) if left else (x - 2.3), y, "F.SilkS", 0.8)
    out.append("\t(embedded_fonts no)\n)\n")
    with open(os.path.join(lib, "ESP32-C3_SuperMini.kicad_mod"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out))
    return lib


def load_footprint(fpid, project_lib):
    lib, name = fpid.split(":")
    path = project_lib if lib == "watch_v1" else os.path.join(FP_DIR, lib + ".pretty")
    fp = pcbnew.FootprintLoad(path, name)
    if fp is None:
        raise SystemExit(f"footprint not found: {fpid}")
    fp.SetFPID(pcbnew.LIB_ID(lib, name))
    return fp


def pad_box(fp):
    xs = [pcbnew.ToMM(p.GetPosition().x) - OX for p in fp.Pads()]
    ys = [pcbnew.ToMM(p.GetPosition().y) - OY for p in fp.Pads()]
    return min(xs), min(ys), max(xs), max(ys)


def pad_xy(fp, number):
    for p in fp.Pads():
        if p.GetNumber() == number:
            return pcbnew.ToMM(p.GetPosition().x) - OX, pcbnew.ToMM(p.GetPosition().y) - OY
    raise KeyError(number)


def put(fp, side, rot, x, y):
    fp.SetPosition(pt(0, 0))
    if side == BOT and fp.GetLayer() == pcbnew.F_Cu:
        fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
    fp.SetOrientationDegrees(rot)
    x0, y0, _, _ = pad_box(fp)
    pos = fp.GetPosition()
    fp.SetPosition(pcbnew.VECTOR2I(pos.x + mm(x - x0), pos.y + mm(y - y0)))


def build():
    project_lib = write_supermini_footprint()
    board = pcbnew.BOARD()

    ds = board.GetDesignSettings()
    ds.SetCopperLayerCount(2)
    default = ds.m_NetSettings.GetDefaultNetclass()
    default.SetClearance(mm(0.2))
    default.SetTrackWidth(mm(0.3))
    default.SetViaDiameter(mm(0.7))
    default.SetViaDrill(mm(0.35))
    power = pcbnew.NETCLASS("Power")
    power.SetClearance(mm(0.2))
    power.SetTrackWidth(mm(0.4))
    power.SetViaDiameter(mm(0.8))
    power.SetViaDrill(mm(0.4))
    ds.m_NetSettings.SetNetclass("Power", power)
    for n in POWER_NETS:
        ds.m_NetSettings.SetNetclassPatternAssignment("/" + n, "Power")

    netinfo = {}
    for name in sorted(nets()):
        ni = pcbnew.NETINFO_ITEM(board, "/" + name)       # root-sheet label -> "/NAME", as eeschema names it
        board.Add(ni)
        netinfo[name] = ni

    fps = {}
    for p in PARTS:
        ref = p["ref"]
        fp = load_footprint(p["footprint"], project_lib)
        board.Add(fp)
        fp.SetReference(ref)
        fp.SetValue(p["value"])
        fp.SetPath(pcbnew.KIID_PATH("/" + uid("sym", ref)))
        side, rot, x, y = PLACE[ref]
        if ref == "U1":
            for rot in (90, 270):
                put(fp, side, rot, x, y)
                if pad_xy(fp, "1")[0] > pad_xy(fp, "8")[0]:      # pads 1 and 16 are at the USB end
                    break
        else:
            put(fp, side, rot, x, y)
        for pad in fp.Pads():
            net = p["pins"].get(pad.GetNumber())
            if net:
                pad.SetNet(netinfo[net])
        if ref.startswith("H"):
            fp.Reference().SetVisible(False)
        fps[ref] = fp

    for a, b in (((0, 0), (BOARD_W, 0)), ((BOARD_W, 0), (BOARD_W, BOARD_H)),
                 ((BOARD_W, BOARD_H), (0, BOARD_H)), ((0, BOARD_H), (0, 0))):
        seg = pcbnew.PCB_SHAPE(board)
        seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
        seg.SetStart(pt(*a))
        seg.SetEnd(pt(*b))
        seg.SetLayer(pcbnew.Edge_Cuts)
        seg.SetWidth(mm(0.1))
        board.Add(seg)

    for text, x, y, side, size in SILK:
        t = pcbnew.PCB_TEXT(board)
        t.SetText(text)
        t.SetPosition(pt(x, y))
        t.SetLayer(pcbnew.F_SilkS if side == TOP else pcbnew.B_SilkS)
        t.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
        t.SetTextThickness(mm(0.15))
        if side == BOT:
            t.SetMirrored(True)
        board.Add(t)
    return board, fps


def check(fps):
    """Assert the things that would be expensive to get wrong."""
    ok = True

    def expect(cond, msg):
        nonlocal ok
        print(("  ok   " if cond else "  FAIL ") + msg)
        ok = ok and cond

    x1, y1 = pad_xy(fps["DS1"], "1")
    x6, _ = pad_xy(fps["DS1"], "6")
    _, y12 = pad_xy(fps["DS1"], "12")
    expect(abs(x1 - 22.65) < 0.01 and abs(y1 - 23.12) < 0.01 and x6 > x1 and y12 < y1,
           "display pin 1 is bottom-left, pins 1-6 run left to right along the bottom")
    u1 = fps["U1"]
    expect(pad_xy(u1, "1")[0] > 55 and pad_xy(u1, "16")[0] > 55, "SuperMini USB-C end is at the right board edge")
    for ref, fp in fps.items():
        x0, y0, x2, y2 = pad_box(fp)
        expect(x0 > 1.0 and y0 > 1.0 and x2 < BOARD_W - 1.0 and y2 < BOARD_H - 1.0, f"{ref} pads inside the outline")
    for ref in [f"R{i}" for i in range(1, 9)]:
        expect(pad_xy(fps[ref], "2")[1] > pad_xy(fps[ref], "1")[1], f"{ref} segment-side pad faces the display")
    return ok


def add_routes(board, pads_path):
    """Add the tracks from route.py, if they belong to this placement."""
    routes_path = os.path.join(OUT_DIR, "watch_v1.routes.json")
    if not os.path.exists(routes_path):
        return "no routes yet - run route.py, then this script again"
    with open(routes_path, encoding="utf-8") as f:
        routes = json.load(f)
    with open(pads_path, "rb") as f:
        if hashlib.sha1(f.read()).hexdigest() != routes["pads_sha1"]:
            return "routes are for an older placement - run route.py, then this script again"
    for s in routes["tracks"]:
        t = pcbnew.PCB_TRACK(board)
        t.SetStart(pt(s["x1"], s["y1"]))
        t.SetEnd(pt(s["x2"], s["y2"]))
        t.SetWidth(mm(s["width"]))
        t.SetLayer(pcbnew.F_Cu if s["layer"] == 0 else pcbnew.B_Cu)
        t.SetNet(board.FindNet("/" + s["net"]))
        board.Add(t)
    for v in routes["vias"]:
        via = pcbnew.PCB_VIA(board)
        via.SetViaType(pcbnew.VIATYPE_THROUGH)
        via.SetPosition(pt(v["x"], v["y"]))
        via.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
        try:
            via.SetWidth(pcbnew.F_Cu, mm(routes["via"]["diameter"]))
        except TypeError:
            via.SetWidth(mm(routes["via"]["diameter"]))
        via.SetDrill(mm(routes["via"]["drill"]))
        via.SetNet(board.FindNet("/" + v["net"]))
        board.Add(via)
    return f"routed: {len(routes['tracks'])} track segments, {len(routes['vias'])} vias"


def dump_pads(board, path):
    """Pad geometry for route.py, in board-relative millimetres."""
    pads = []
    for fp in sorted(board.GetFootprints(), key=lambda f: f.GetReference()):
        for pad in fp.Pads():
            box = pad.GetBoundingBox()
            pads.append(dict(
                ref=fp.GetReference(), num=pad.GetNumber(), net=pad.GetNetname().lstrip("/"),
                x=round(pcbnew.ToMM(pad.GetPosition().x) - OX, 4), y=round(pcbnew.ToMM(pad.GetPosition().y) - OY, 4),
                w=round(pcbnew.ToMM(box.GetWidth()), 4), h=round(pcbnew.ToMM(box.GetHeight()), 4),
                drill=round(pcbnew.ToMM(pad.GetDrillSize().x), 4),
                plated=pad.GetAttribute() == pcbnew.PAD_ATTRIB_PTH))
    with open(path, "w", encoding="utf-8") as f:
        json.dump(dict(width=BOARD_W, height=BOARD_H, pads=pads), f, indent=1)


def write_project():
    path = os.path.join(OUT_DIR, "watch_v1.kicad_pro")
    with open(path, encoding="utf-8") as f:
        pro = json.load(f)
    base = dict(pro["net_settings"]["classes"][0])
    base.update(name="Default", clearance=0.2, track_width=0.3, via_diameter=0.7, via_drill=0.35)
    power = dict(base, name="Power", track_width=0.4, via_diameter=0.8, via_drill=0.4, priority=0)
    pro["net_settings"]["classes"] = [base, power]
    pro["net_settings"]["netclass_patterns"] = [{"netclass": "Power", "pattern": "/" + n} for n in POWER_NETS]
    rules = pro["board"].setdefault("design_settings", {}).setdefault("rules", {})
    rules.update(min_clearance=0.15, min_track_width=0.2, min_via_diameter=0.5, min_via_annular_width=0.05,
                 min_through_hole_diameter=0.3, min_hole_to_hole=0.25, min_copper_edge_clearance=0.3)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(pro, f, indent=2)
        f.write("\n")


def main():
    board, fps = build()
    good = check(fps)
    out = os.path.join(OUT_DIR, "watch_v1.kicad_pcb")
    pads_path = os.path.join(OUT_DIR, "watch_v1.pads.json")
    dump_pads(board, pads_path)
    print(add_routes(board, pads_path))
    pcbnew.SaveBoard(out, board)
    if os.path.exists(os.path.join(OUT_DIR, "watch_v1.kicad_pro")):
        write_project()
    print("saved", out, "footprints:", len(fps))
    if not good:
        raise SystemExit("placement checks failed")


if __name__ == "__main__":
    try:
        main()
        code = 0
    except SystemExit as e:
        print(e)
        code = 1
    sys.stdout.flush()
    os._exit(code)      # KiCad's Python can hang on a normal interpreter shutdown
