"""Generate kicad/pack_base.kicad_sch: the Pack Base schematic.

Run with any Python 3 (KiCad's own works too):   python gen_pack_base_sch.py

The parts and their nets come from gen_pack_base.py (the same table that builds
the board), so the schematic and the PCB cannot disagree. The sheet is drawn the
same way as the watch's: every pin has a short wire and a net label, and pins
with the same label are connected.
"""
import os
import sys
import types

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "scripts"))
sys.path.insert(0, HERE)

# gen_pack_base.py imports pcbnew at the top but only uses it when it builds the
# board. Plain Python has no pcbnew, so give it an empty stand-in to read PARTS.
if "pcbnew" not in sys.modules:
    try:
        import pcbnew  # noqa: F401
    except ImportError:
        sys.modules["pcbnew"] = types.ModuleType("pcbnew")

import gen_pack_base as pb  # noqa: E402
import gen_sch  # noqa: E402
from sexpr import dump  # noqa: E402

PROJECT = "pack_base"

SYMBOLS = {
    "J1": ("Connector_Generic:Conn_02x03_Odd_Even", "Watch plug (underneath)"),
    "J2": ("Connector_Generic:Conn_01x06", "Breakout row"),
    "J3": ("Connector_Generic:Conn_01x03", "Microphone row"),
    "J5": ("Connector_Generic_MountingPin:Conn_01x04_MountingPin", "Qwiic"),
    "JP1": ("Jumper:SolderJumper_3_Bridged12", "Sensor power"),
}

POS = {"J1": (50.8, 76.2), "J2": (114.3, 76.2), "J3": (177.8, 76.2),
       "J5": (50.8, 134.62), "JP1": (114.3, 134.62)}

TITLES = [
    (30.48, 58.42, "J1: to the watch's socket J4"),
    (96.52, 58.42, "J2: breakout board row"),
    (160.02, 58.42, "J3: microphone board row"),
    (30.48, 116.84, "J5: Qwiic / STEMMA QT socket"),
    (96.52, 116.84, "JP1: sensor power select"),
    (30.48, 175.26, "Power flags (ERC only)"),
]

NOTES = [
    (30.48, 88.9,
     "J1 is soldered on the BOTTOM side.\n"
     "Pin numbers match the watch's J4:\n"
     "1 GND  3 SDA  5 AUX (3.3 V, awake)\n"
     "2 3V3  4 SCL  6 AIN (audio in)"),
    (96.52, 88.9,
     "Order V G SCL SDA, as on most cheap\n"
     "I2C boards (BME280, BH1750, AHT20,\n"
     "GY-521). Many OLED boards swap V and G\n"
     "(G V SCL SDA): check before soldering."),
    (160.02, 88.9,
     "MAX4466 / MAX9814: OUT, VCC, GND.\n"
     "VCC is AUX, so the microphone is\n"
     "off while the watch sleeps."),
    (30.48, 152.4,
     "Qwiic pinout: 1 GND, 2 3V3, 3 SDA, 4 SCL.\n"
     "MP = the two metal tabs, solder only."),
    (96.52, 152.4,
     "Pads 1-2 bridged at the factory: PWR = 3V3 (always on).\n"
     "Cut that bridge and blob 2-3: PWR = AUX (on only while the\n"
     "watch is awake) - for modules with a power LED, MPU6050..."),
]

NOTE = "Pins with the same label are connected. Generated from hardware/addons/pack_base/gen_pack_base.py - edit that and regenerate."


def parts():
    out = []
    for ref, (footprint, _side, pins, _anchor, _test) in pb.PARTS.items():
        symbol, value = SYMBOLS[ref]
        out.append(dict(ref=ref, symbol=symbol, value=value, footprint=footprint, pins=pins))
    return out


def main():
    sch = gen_sch.build(parts=parts(), pos=POS, titles=TITLES, notes=NOTES, power_nets=["+3V3", "GND"],
                        project=PROJECT, title="Watch V1 - Pack Base (add-on carrier)",
                        rev=pb.REV, note=NOTE, paper="A4", key=PROJECT, flag_y=185.42)
    os.makedirs(pb.OUT_DIR, exist_ok=True)
    with open(os.path.join(pb.OUT_DIR, PROJECT + ".kicad_sch"), "w", encoding="utf-8", newline="\n") as f:
        f.write(dump(sch) + "\n")
    print("wrote", PROJECT + ".kicad_sch with", len(pb.PARTS), "parts")


if __name__ == "__main__":
    main()
