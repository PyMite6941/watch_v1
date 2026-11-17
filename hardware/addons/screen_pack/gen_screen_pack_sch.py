"""Generate kicad/screen_pack.kicad_sch: the Screen Pack schematic.

Run with any Python 3 (KiCad's own works too):   python gen_screen_pack_sch.py

The parts and nets come from gen_screen_pack.py (the table that builds the board),
so the schematic and the PCB cannot disagree.
"""
import os
import sys
import types

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "scripts"))
sys.path.insert(0, HERE)

if "pcbnew" not in sys.modules:
    try:
        import pcbnew  # noqa: F401
    except ImportError:
        sys.modules["pcbnew"] = types.ModuleType("pcbnew")

import gen_screen_pack as sp  # noqa: E402
import gen_sch  # noqa: E402
from sexpr import dump  # noqa: E402

PROJECT = "screen_pack"

SYMBOLS = {
    "J1": ("Connector_Generic:Conn_02x03_Odd_Even", "Watch plug (underneath)"),
    "J2": ("Connector_Generic:Conn_01x04", "OLED row A (G V CL DA)"),
    "J3": ("Connector_Generic:Conn_01x04", "OLED row B (V G CL DA)"),
}

POS = {"J1": (50.8, 76.2), "J2": (114.3, 76.2), "J3": (177.8, 76.2)}

TITLES = [
    (30.48, 58.42, "J1: to the watch's socket J4"),
    (96.52, 58.42, "J2: OLED row A, GND first"),
    (160.02, 58.42, "J3: OLED row B, VCC first"),
    (30.48, 175.26, "Power flags (ERC only)"),
]

NOTES = [
    (30.48, 88.9,
     "J1 is soldered on the BOTTOM side.\n"
     "Pin numbers match the watch's J4:\n"
     "1 GND  3 SDA  5 AUX (not used here)\n"
     "2 3V3  4 SCL  6 AIN (not used here)"),
    (96.52, 88.9,
     "Most 0.91\" (128x32) and 0.96\" (128x64)\n"
     "SSD1306 I2C modules: GND VCC SCL SDA.\n"
     "Fit a 4-pin female header in ONE row:\n"
     "the one whose order matches the module."),
    (160.02, 88.9,
     "Some modules swap power: VCC GND SCL SDA.\n"
     "Those go in this row. Never fit both rows:\n"
     "a module in the wrong row gets 3V3 on GND.\n"
     "The module outline was not verified."),
]

NOTE = "Pins with the same label are connected. Generated from hardware/addons/screen_pack/gen_screen_pack.py - edit that and regenerate."


def parts():
    out = []
    for ref, (footprint, _side, pins, _anchor, _test) in sp.PARTS.items():
        symbol, value = SYMBOLS[ref]
        out.append(dict(ref=ref, symbol=symbol, value=value, footprint=footprint, pins=pins))
    return out


def main():
    sch = gen_sch.build(parts=parts(), pos=POS, titles=TITLES, notes=NOTES, power_nets=["+3V3", "GND"],
                        project=PROJECT, title="Watch V1 - Screen Pack (OLED carrier)",
                        rev=sp.REV, note=NOTE, paper="A4", key=PROJECT, flag_y=185.42)
    os.makedirs(sp.OUT_DIR, exist_ok=True)
    with open(os.path.join(sp.OUT_DIR, PROJECT + ".kicad_sch"), "w", encoding="utf-8", newline="\n") as f:
        f.write(dump(sch) + "\n")
    print("wrote", PROJECT + ".kicad_sch with", len(sp.PARTS), "parts")


if __name__ == "__main__":
    main()
