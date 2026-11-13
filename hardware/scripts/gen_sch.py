"""Generate hardware/kicad/watch_v1.kicad_sch (+ project symbol library) from design.py.

Run with any Python 3:   python gen_sch.py
Every pin gets a short wire stub and a net label, so the sheet reads like an
annotated netlist: two pins are connected when they carry the same label.
"""
import copy
import os
import uuid

from design import PARTS, POWER_NETS, SUPERMINI_PINS
from sexpr import Sym, dump, find, first, parse

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.join(HERE, "..", "kicad")
KICAD_SYMBOLS = os.environ.get(
    "KICAD_SYMBOL_DIR",
    os.path.expandvars(r"%LOCALAPPDATA%\Programs\KiCad\10.0\share\kicad\symbols"),
)
PROJECT = "watch_v1"
NS = uuid.UUID("6f1d3c7e-52a4-4b7b-9a53-77a1c0de0001")
STUB = 5.08


def uid(*key):
    return str(uuid.uuid5(NS, "/".join(str(k) for k in key)))


ROOT_UUID = uid("root")
S = Sym


def font(size=1.27):
    return [S("effects"), [S("font"), [S("size"), size, size]]]


# --------------------------------------------------------------- custom symbol
def supermini_symbol():
    def prop(name, value, x, y, hide=False):
        node = [S("property"), name, value, [S("at"), x, y, 0]]
        if hide:
            node.append([S("hide"), S("yes")])
        node.append(font())
        return node

    pins = []
    for n in range(1, 17):
        left = n <= 8
        row = n - 1 if left else 16 - n
        pins.append([
            S("pin"), S("passive"), S("line"),
            [S("at"), -15.24 if left else 15.24, 8.89 - row * 2.54, 0 if left else 180],
            [S("length"), 2.54],
            [S("name"), SUPERMINI_PINS[str(n)], font()],
            [S("number"), str(n), font()],
        ])
    name = "ESP32-C3_SuperMini"
    return [
        S("symbol"), name,
        [S("exclude_from_sim"), S("no")], [S("in_bom"), S("yes")], [S("on_board"), S("yes")],
        prop("Reference", "U", 0, 15.24),
        prop("Value", name, 0, -15.24),
        prop("Footprint", "watch_v1:ESP32-C3_SuperMini", 0, -17.78, hide=True),
        prop("Datasheet", "https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf", 0, -20.32, hide=True),
        prop("Description", "ESP32-C3 SuperMini module, 2x8 pins, 15.24 mm row spacing, USB-C", 0, -22.86, hide=True),
        [S("symbol"), name + "_0_1",
         [S("rectangle"), [S("start"), -12.7, 12.7], [S("end"), 12.7, -12.7],
          [S("stroke"), [S("width"), 0.254], [S("type"), S("default")]],
          [S("fill"), [S("type"), S("background")]]],
         [S("text"), "USB-C", [S("at"), 0, 10.16, 0], font()]],
        [S("symbol"), name + "_1_1"] + pins,
    ]


# ------------------------------------------------------------ library loading
_lib_cache = {}


def load_lib_symbol(lib_id):
    lib, name = lib_id.split(":")
    if lib == PROJECT:
        return supermini_symbol()
    if lib not in _lib_cache:
        path = os.path.join(KICAD_SYMBOLS, lib + ".kicad_sym")
        _lib_cache[lib] = {s[1]: s for s in find(parse(open(path, encoding="utf-8").read()), "symbol")}
    node = copy.deepcopy(_lib_cache[lib][name])
    ext = first(node, "extends")
    if ext:  # flatten a derived symbol onto its parent
        parent = copy.deepcopy(_lib_cache[lib][ext[1]])
        parent[1] = name
        for sub in find(parent, "symbol"):
            sub[1] = name + sub[1][len(ext[1]):]
        child_props = {p[1]: p for p in find(node, "property")}
        parent[:] = [x for x in parent if not (isinstance(x, list) and x[0] == "property")]
        for p in child_props.values():
            parent.append(p)
        node = parent
    return node


def symbol_pins(node):
    """{number: (x, y, angle, unit)} in symbol coordinates (y up)."""
    out = {}
    for sub in find(node, "symbol"):
        unit = int(sub[1].rsplit("_", 2)[1])
        for p in find(sub, "pin"):
            at = first(p, "at")
            out[first(p, "number")[1]] = (float(at[1]), float(at[2]), int(float(at[3])), unit)
    return out


# ------------------------------------------------------------------ placement
POS = {
    "U1": (63.5, 88.9), "U2": (139.7, 71.12), "U3": (139.7, 152.4),
    "DS1": (330.2, 71.12),
    "R13": (284.48, 139.7), "Q5": (284.48, 172.72), "D2": (317.5, 162.56), "J3": (350.52, 162.56),
    "D3": (187.96, 220.98), "R14": (223.52, 220.98), "R15": (236.22, 220.98), "R16": (248.92, 220.98),
    "R17": (101.6, 152.4), "SW1": (48.26, 157.48), "SW2": (48.26, 177.8),
    "R18": (48.26, 220.98), "R19": (60.96, 220.98), "C3": (73.66, 220.98),
    "R21": (99.06, 220.98), "R20": (111.76, 220.98),
    "C1": (137.16, 220.98), "C2": (149.86, 220.98),
    "D1": (294.64, 220.98), "J2": (335.28, 220.98), "J1": (259.08, 259.08), "J4": (309.88, 259.08), "C4": (350.52, 259.08),
}
for i in range(8):
    POS[f"R{i + 1}"] = (182.88 + 10.16 * i, 71.12)
for i in range(4):
    POS[f"R{9 + i}"] = (177.8 + 25.4 * i, 139.7)
    POS[f"Q{1 + i}"] = (177.8 + 25.4 * i, 172.72)
    POS[f"H{i + 1}"] = (182.88 + 12.7 * i, 259.08)

TITLES = [
    (30.48, 55.88, "MCU: ESP32-C3 SuperMini (only GPIO0-10, 20, 21 exist)"),
    (116.84, 45.72, "Segment driver: 74HC595 + 8x 220R"),
    (116.84, 127.0, "Digit / LED / haptic driver: 74HC595 + NPN low-side switches"),
    (294.64, 50.8, "Display: 4-digit common cathode"),
    (30.48, 147.32, "Buttons"),
    (30.48, 205.74, "Battery sense"),
    (88.9, 205.74, "Light sensor"),
    (127.0, 205.74, "Decoupling"),
    (172.72, 205.74, "RGB LED"),
    (279.4, 205.74, "Power: Li-Po -> D1 -> 5V pin. TP4056 module (own USB port) on J2"),
    (294.64, 246.38, "Accessory port (I2C + switched power + analogue in)"),
    (243.84, 246.38, "DS3231 RTC module"),
    (279.4, 127.0, "Haptic motor"),
    (30.48, 256.54, "Power flags (ERC only)"),
    (172.72, 248.92, "Mounting holes"),
]

OUT_DIR_VEC = {0: (-1, 0), 180: (1, 0), 90: (0, 1), 270: (0, -1)}  # sheet direction away from body
LABEL_STYLE = {(-1, 0): (180, "right"), (1, 0): (0, "left"), (0, -1): (90, "left"), (0, 1): (270, "right")}


def r(v):
    return round(v, 4)


def build():
    items = []
    lib_symbols = {}

    def add_symbol(ref, lib_id, value, footprint, pins, x, y, in_bom=True):
        if lib_id not in lib_symbols:
            node = load_lib_symbol(lib_id)
            node[1] = lib_id
            lib_symbols[lib_id] = node
        lib = lib_symbols[lib_id]
        pin_geo = symbol_pins(lib)
        inst = [
            S("symbol"), [S("lib_id"), lib_id], [S("at"), x, y, 0], [S("unit"), 1],
            [S("exclude_from_sim"), S("no")], [S("in_bom"), S("yes" if in_bom else "no")],
            [S("on_board"), S("yes")], [S("dnp"), S("no")], [S("uuid"), uid("sym", ref)],
        ]
        overrides = {"Reference": ref, "Value": value, "Footprint": footprint}
        for p in find(lib, "property"):
            name = p[1]
            if name not in ("Reference", "Value", "Footprint", "Datasheet", "Description"):
                continue
            at = first(p, "at")
            node = [S("property"), name, overrides.get(name, p[2]),
                    [S("at"), r(x + float(at[1])), r(y - float(at[2])), int(float(at[3]))]]
            hidden = first(p, "hide") is not None or name in ("Footprint", "Datasheet", "Description")
            if hidden:
                node.append([S("hide"), S("yes")])
            eff = copy.deepcopy(first(p, "effects")) or font()
            eff[:] = [e for e in eff if not (isinstance(e, list) and e[0] == "hide") and e != "hide"]
            node.append(eff)
            inst.append(node)
        for num in pin_geo:
            inst.append([S("pin"), num, [S("uuid"), uid("pin", ref, num)]])
        inst.append([S("instances"), [S("project"), PROJECT,
                    [S("path"), "/" + ROOT_UUID, [S("reference"), ref], [S("unit"), 1]]]])
        items.append(inst)

        for num, (px, py, ang, _unit) in pin_geo.items():
            ex, ey = r(x + px), r(y - py)
            if num not in pins:
                raise SystemExit(f"{ref}: pin {num} missing from design.py")
            net = pins[num]
            if net is None:
                items.append([S("no_connect"), [S("at"), ex, ey], [S("uuid"), uid("nc", ref, num)]])
                continue
            dx, dy = OUT_DIR_VEC[ang]
            lx, ly = r(ex + dx * STUB), r(ey + dy * STUB)
            items.append([S("wire"), [S("pts"), [S("xy"), ex, ey], [S("xy"), lx, ly]],
                          [S("stroke"), [S("width"), 0], [S("type"), S("default")]],
                          [S("uuid"), uid("wire", ref, num)]])
            angle, just = LABEL_STYLE[(dx, dy)]
            eff = font()
            eff.append([S("justify"), S(just), S("bottom")])
            items.append([S("label"), net, [S("at"), lx, ly, angle], eff, [S("uuid"), uid("label", ref, num)]])

    for p in PARTS:
        x, y = POS[p["ref"]]
        add_symbol(p["ref"], p["symbol"], p["value"], p["footprint"], p["pins"], x, y,
                   in_bom=not p["ref"].startswith("H"))
    for i, net in enumerate(POWER_NETS):
        add_symbol(f"#FLG0{i + 1}", "power:PWR_FLAG", "PWR_FLAG", "", {"1": net}, 48.26 + 20.32 * i, 266.7, in_bom=False)

    for x, y, text in TITLES:
        eff = [S("effects"), [S("font"), [S("size"), 2, 2], [S("bold"), S("yes")]],
               [S("justify"), S("left"), S("bottom")]]
        items.append([S("text"), text, [S("exclude_from_sim"), S("no")], [S("at"), x, y, 0], eff,
                      [S("uuid"), uid("text", text)]])
    note = "Pins with the same label are connected. Generated from hardware/scripts/design.py - edit that and regenerate."
    items.append([S("text"), note, [S("exclude_from_sim"), S("no")], [S("at"), 30.48, 38.1, 0],
                  [S("effects"), [S("font"), [S("size"), 1.5, 1.5]], [S("justify"), S("left"), S("bottom")]],
                  [S("uuid"), uid("text", "note")]])

    sch = [
        S("kicad_sch"), [S("version"), 20250610], [S("generator"), "eeschema"], [S("generator_version"), "10.0"],
        [S("uuid"), ROOT_UUID], [S("paper"), "A3"],
        [S("title_block"), [S("title"), "Watch V1 - ESP32-C3 SuperMini LED watch"],
         [S("date"), "2026-11-12"], [S("rev"), "5"]],
        [S("lib_symbols")] + list(lib_symbols.values()),
    ] + items + [
        [S("sheet_instances"), [S("path"), "/", [S("page"), "1"]]],
        [S("embedded_fonts"), S("no")],
    ]
    return sch


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    with open(os.path.join(OUT_DIR, PROJECT + ".kicad_sch"), "w", encoding="utf-8", newline="\n") as f:
        f.write(dump(build()) + "\n")
    lib = [S("kicad_symbol_lib"), [S("version"), 20241209], [S("generator"), "watch_v1_gen"],
           [S("generator_version"), "10.0"], supermini_symbol()]
    with open(os.path.join(OUT_DIR, PROJECT + ".kicad_sym"), "w", encoding="utf-8", newline="\n") as f:
        f.write(dump(lib) + "\n")
    with open(os.path.join(OUT_DIR, "sym-lib-table"), "w", encoding="utf-8", newline="\n") as f:
        f.write('(sym_lib_table\n  (version 7)\n  (lib (name "watch_v1")(type "KiCad")'
                '(uri "${KIPRJMOD}/watch_v1.kicad_sym")(options "")(descr "Watch V1 project symbols"))\n)\n')
    with open(os.path.join(OUT_DIR, "fp-lib-table"), "w", encoding="utf-8", newline="\n") as f:
        f.write('(fp_lib_table\n  (version 7)\n  (lib (name "watch_v1")(type "KiCad")'
                '(uri "${KIPRJMOD}/watch_v1.pretty")(options "")(descr "Watch V1 project footprints"))\n)\n')
    print("wrote schematic with", len(PARTS), "parts")


if __name__ == "__main__":
    main()
