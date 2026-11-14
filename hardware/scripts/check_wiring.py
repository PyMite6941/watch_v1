"""Independent audit of the wiring. Run after gen_sch.py and gen_pcb.py:

    python check_wiring.py

It does not trust design.py. It reads the netlist KiCad itself extracts from
the generated schematic, and the pad nets of the generated board, and checks
them against the pinouts in the parts' datasheets (written out below) and
against what the firmware expects. Anything that disagrees is printed as FAIL.
"""
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

from sexpr import find, first, parse

HERE = os.path.dirname(os.path.abspath(__file__))
KICAD = os.path.join(HERE, "..", "kicad")
CONFIG_H = os.path.join(HERE, "..", "..", "firmware", "watch_v1", "config.h")
CLI = os.environ.get("KICAD_CLI") or shutil.which("kicad-cli") or os.path.expandvars(
    r"%LOCALAPPDATA%\Programs\KiCad\10.0\bin\kicad-cli.exe")

# ---- datasheet pinouts ------------------------------------------------------
# ESP32-C3 SuperMini, component side up, USB-C at the top. Pads numbered like a DIP.
SUPERMINI = {1: "GPIO5", 2: "GPIO6", 3: "GPIO7", 4: "GPIO8", 5: "GPIO9", 6: "GPIO10", 7: "GPIO20", 8: "GPIO21",
             9: "GPIO0", 10: "GPIO1", 11: "GPIO2", 12: "GPIO3", 13: "GPIO4", 14: "3V3", 15: "GND", 16: "5V"}
# 74HC595: outputs QA..QH, then the control pins.
HC595_Q = {"QA": 15, "QB": 1, "QC": 2, "QD": 3, "QE": 4, "QF": 5, "QG": 6, "QH": 7}
HC595 = {"SER": 14, "SRCLK": 11, "RCLK": 12, "OE": 13, "SRCLR": 10, "VCC": 16, "GND": 8, "QH'": 9}
# 5641AS / 5643AS / CC56-12 four-digit common-cathode display.
DISPLAY = {"A": 11, "B": 7, "C": 4, "D": 2, "E": 1, "F": 10, "G": 5, "DP": 3, "DIG1": 12, "DIG2": 9, "DIG3": 8, "DIG4": 6}

failures = 0


def check(ok, text):
    global failures
    print(("  ok    " if ok else "  FAIL  ") + text)
    failures += not ok


def schematic_nets():
    """{net name: {(ref, pin)}} as KiCad extracts it from the schematic."""
    out_file = os.path.join(tempfile.mkdtemp(), "watch_v1.net")
    subprocess.run([CLI, "sch", "export", "netlist", "--format", "kicadsexpr", "-o", out_file,
                    os.path.join(KICAD, "watch_v1.kicad_sch")], check=True, stdout=subprocess.DEVNULL)
    tree = parse(open(out_file, encoding="utf-8").read())
    nets = {}
    for net in find(first(tree, "nets"), "net"):
        name = first(net, "name")[1].lstrip("/")
        nets[name] = {(first(n, "ref")[1], first(n, "pin")[1]) for n in find(net, "node")}
    return nets


def main():
    nets = schematic_nets()
    where = {}                                        # (ref, pin) -> net
    for name, nodes in nets.items():
        for node in nodes:
            where[node] = name

    def net(ref, pin):
        return where.get((ref, str(pin)), "(not connected)")

    def joined(a, b):
        return net(*a) == net(*b) and not net(*a).startswith("unconnected") and net(*a) != "(not connected)"

    def through(a, part, b):
        """a - [two-pin part] - b: the part sits between the two points."""
        ends = {net(part, 1), net(part, 2)}
        return ends == {net(*a), net(*b)} and len(ends) == 2

    print("Firmware pins (config.h) against the SuperMini pads")
    gpio_pad = {int(v[4:]): k for k, v in SUPERMINI.items() if v.startswith("GPIO")}
    pins = dict(re.findall(r"#define\s+(PIN_\w+)\s+(\d+)", open(CONFIG_H, encoding="utf-8").read()))
    expected = {"PIN_VBAT_SENSE": "VBAT_SENSE", "PIN_LDR_SENSE": "LDR_SENSE", "PIN_BTN_MODE": "BTN_MODE",
                "PIN_BTN_SET": "BTN_SET", "PIN_CHRG": "CHRG", "PIN_SR_DATA": "SR_DATA", "PIN_SR_CLK": "SR_CLK",
                "PIN_SDA": "SDA", "PIN_SCL": "SCL", "PIN_SR_LATCH": "SR_LATCH", "PIN_SR_OE": "SR_OE",
                "PIN_LDR_PWR": "LDR_PWR"}
    for define, want in expected.items():
        gpio = int(pins[define])
        check(net("U1", gpio_pad[gpio]) == want, f"{define} = GPIO{gpio} = U1 pad {gpio_pad[gpio]} is on net {want}")
    check(net("U1", 11).startswith("unconnected"), "GPIO2 (boot strapping pin) is left unconnected")
    check(net("U1", 14) == "+3V3" and net("U1", 15) == "GND" and net("U1", 16) == "VBUS", "U1 power pads: 3V3, GND, 5V")

    print("Shift registers")
    for u in ("U2", "U3"):
        check(net(u, HC595["SRCLK"]) == "SR_CLK" and net(u, HC595["RCLK"]) == "SR_LATCH" and net(u, HC595["OE"]) == "SR_OE",
              f"{u} clock, latch and output-enable are on the shared lines")
        check(net(u, HC595["VCC"]) == "+3V3" and net(u, HC595["GND"]) == "GND" and net(u, HC595["SRCLR"]) == "+3V3",
              f"{u} powered, and its clear pin is tied high")
    check(net("U2", HC595["SER"]) == "SR_DATA", "U2 data in comes from the processor")
    check(joined(("U2", HC595["QH'"]), ("U3", HC595["SER"])), "U2 serial out feeds U3 data in (daisy chain)")
    check(through(("U1", gpio_pad[20]), "R17", ("U1", 14)), "R17 pulls output-enable up to 3V3 (display off until the firmware runs)")

    print("Display segments: U2 output -> 220 ohm -> display pin   (firmware bit order a..g, dp = QA..QH)")
    for i, (seg, q) in enumerate(zip(["A", "B", "C", "D", "E", "F", "G", "DP"], HC595_Q)):
        check(through(("U2", HC595_Q[q]), f"R{i + 1}", ("DS1", DISPLAY[seg])),
              f"segment {seg:2s}: U2 {q} (pin {HC595_Q[q]}) - R{i + 1} - DS1 pin {DISPLAY[seg]}")

    print("Display digits: U3 output -> 1k -> transistor base; emitter to ground; collector to the digit's cathode")
    for i, q in enumerate(["QA", "QB", "QC", "QD"]):
        tr = f"Q{i + 1}"
        check(through(("U3", HC595_Q[q]), f"R{9 + i}", (tr, 2)) and net(tr, 1) == "GND" and
              joined((tr, 3), ("DS1", DISPLAY[f"DIG{i + 1}"])),
              f"digit {i + 1}: U3 {q} (pin {HC595_Q[q]}) - R{9 + i} - {tr} base; {tr} collector - DS1 pin {DISPLAY[f'DIG{i + 1}']}")

    print("RGB LED and vibration motor on U3 (firmware: QE red, QF green, QG blue, QH motor)")
    for q, r, leg, colour in (("QE", "R14", 1, "red"), ("QF", "R15", 3, "green"), ("QG", "R16", 4, "blue")):
        check(through(("U3", HC595_Q[q]), r, ("D3", leg)), f"{colour}: U3 {q} - {r} - D3 leg {leg}")
    check(net("D3", 2) == "GND", "D3 common cathode (leg 2) to ground")
    check(through(("U3", HC595_Q["QH"]), "R13", ("Q5", 2)) and net("Q5", 1) == "GND", "motor: U3 QH - R13 - Q5 base, emitter to ground")
    check(net("J3", 1) == "+3V3" and joined(("J3", 2), ("Q5", 3)), "motor between 3V3 (J3 +) and Q5 collector (J3 -)")
    check(net("D2", 1) == "+3V3" and joined(("D2", 2), ("Q5", 3)), "D2 across the motor, cathode to 3V3 (flyback)")

    print("Power")
    check(net("D1", 1) == "VBUS" and net("D1", 2) == "VBAT", "D1: anode on the battery, cathode on the SuperMini 5V pin")
    check(net("J2", 1) == "GND" and net("J2", 2) == "VBAT" and net("J2", 3) == "CHRG", "J2 charger pads: GND, battery, charge signal")
    check(nets["VBUS"] == {("U1", "16"), ("D1", "1")}, "nothing but D1 touches the USB 5V pin (the charger is separate)")
    check(through(("J2", 2), "R18", ("U1", gpio_pad[0])) and through(("U1", gpio_pad[0]), "R19", ("U1", 15)),
          "battery sense: battery - R18 - GPIO0 - R19 - ground")
    for c in ("C1", "C2"):
        check({net(c, 1), net(c, 2)} == {"+3V3", "GND"}, f"{c} decouples 3V3")

    print("Buttons, light sensor, I2C, accessory socket")
    for sw, name in (("SW1", "BTN_MODE"), ("SW2", "BTN_SET")):
        check({net(sw, 1), net(sw, 2)} == {name, "GND"}, f"{sw} between {name} and ground")
    check(through(("U1", gpio_pad[21]), "R21", ("U1", gpio_pad[1])) and through(("U1", gpio_pad[1]), "R20", ("U1", 15)),
          "light sensor: GPIO21 - LDR - GPIO1 - R20 - ground")
    check(through(("U1", gpio_pad[8]), "R22", ("U1", 14)) and through(("U1", gpio_pad[9]), "R23", ("U1", 14)),
          "I2C pull-ups R22 (SDA) and R23 (SCL) to 3V3")
    check([net("J1", n) for n in (1, 2, 3, 5)] == ["+3V3", "SDA", "SCL", "GND"], "J1 clock module pads: 3V3, SDA, SCL, -, GND")
    check([net("J4", n) for n in range(1, 7)] == ["GND", "+3V3", "SDA", "SCL", "LDR_PWR", "AIN"],
          "J4 accessory socket: 1 GND, 2 3V3, 3 SDA, 4 SCL, 5 AUX, 6 AIN")
    check(through(("J4", 6), "C4", ("U1", gpio_pad[0])), "C4 couples AIN onto the battery-sense pin")

    print("Board against schematic")
    with open(os.path.join(KICAD, "watch_v1.pads.json"), encoding="utf-8") as f:
        pads = json.load(f)["pads"]
    board = {}
    for p in pads:
        if p["net"]:
            board.setdefault(p["net"], set()).add((p["ref"], p["num"]))
    schematic = {k: v for k, v in nets.items() if not k.startswith("unconnected")}
    check(board == schematic, f"every pad on the board is on the same net as in the schematic ({len(schematic)} nets)")
    check(all(len(v) >= 2 for v in schematic.values()), "no net has only one pin")

    print(f"\n{'ALL CHECKS PASSED' if not failures else str(failures) + ' CHECK(S) FAILED'}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
