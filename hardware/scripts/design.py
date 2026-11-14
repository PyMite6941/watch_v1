"""Watch V1 - the single source of truth for the circuit.

gen_sch.py turns this into the KiCad schematic, gen_pcb.py into the board.
Each part: reference, library symbol, value, footprint, {pin number: net}.
A net of None means "deliberately not connected".
"""

R_V = "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical"
TO92 = "Package_TO_SOT_THT:TO-92_Inline"
DIP16 = "Package_DIP:DIP-16_W7.62mm"
CAP = "Capacitor_THT:C_Disc_D3.0mm_W1.6mm_P2.50mm"

PARTS = []


def part(ref, symbol, value, footprint, pins, note=""):
    PARTS.append(dict(ref=ref, symbol=symbol, value=value, footprint=footprint, pins=pins, note=note))


# ---- MCU module. Pad numbers run like a DIP seen from the module's top, USB up.
part("U1", "watch_v1:ESP32-C3_SuperMini", "ESP32-C3 SuperMini", "watch_v1:ESP32-C3_SuperMini", {
    "1": "CHRG",        # GPIO5
    "2": "SR_DATA",     # GPIO6
    "3": "SR_CLK",      # GPIO7
    "4": "SDA",         # GPIO8
    "5": "SCL",         # GPIO9
    "6": "SR_LATCH",    # GPIO10
    "7": "SR_OE",       # GPIO20
    "8": "LDR_PWR",     # GPIO21
    "9": "VBAT_SENSE",  # GPIO0
    "10": "LDR_SENSE",  # GPIO1
    "11": None,         # GPIO2 (strapping pin, unused)
    "12": "BTN_MODE",   # GPIO3
    "13": "BTN_SET",    # GPIO4
    "14": "+3V3",
    "15": "GND",
    "16": "VBUS",       # "5V" pin
})

# ---- Shift registers. U2 drives the segments, U3 the digits, RGB LED and motor.
SEG = "ABCDEFG"
sr_common = {"11": "SR_CLK", "12": "SR_LATCH", "13": "SR_OE", "10": "+3V3", "16": "+3V3", "8": "GND"}
part("U2", "74xx:74HC595", "74HC595", DIP16, dict(sr_common, **{
    "14": "SR_DATA", "9": "SR_CHAIN",
    "15": "SR_A", "1": "SR_B", "2": "SR_C", "3": "SR_D", "4": "SR_E", "5": "SR_F", "6": "SR_G", "7": "SR_DP",
}))
part("U3", "74xx:74HC595", "74HC595", DIP16, dict(sr_common, **{
    "14": "SR_CHAIN", "9": None,
    "15": "DIG1_DRV", "1": "DIG2_DRV", "2": "DIG3_DRV", "3": "DIG4_DRV",
    "4": "LED_R_DRV", "5": "LED_G_DRV", "6": "LED_B_DRV", "7": "HAPTIC_DRV",
}))
part("C1", "Device:C", "100nF", CAP, {"1": "+3V3", "2": "GND"})
part("C2", "Device:C", "100nF", CAP, {"1": "+3V3", "2": "GND"})

# ---- Display: 0.56" 4-digit common cathode, 12 pins (5641AS / 5643 / CC56-12 pinout).
part("DS1", "Display_Character:CC56-12GWA", "5643A-1 (CC, 0.56in)", "Display_7Segment:CC56-12GWA", {
    "11": "SEG_A", "7": "SEG_B", "4": "SEG_C", "2": "SEG_D", "1": "SEG_E", "10": "SEG_F", "5": "SEG_G",
    "3": "SEG_DP", "12": "DIG1", "9": "DIG2", "8": "DIG3", "6": "DIG4",
})
for i, s in enumerate(list(SEG) + ["DP"]):
    part(f"R{i + 1}", "Device:R", "220", R_V, {"1": f"SR_{s}", "2": f"SEG_{s}"})
for i in range(4):
    part(f"R{9 + i}", "Device:R", "1k", R_V, {"1": f"DIG{i + 1}_DRV", "2": f"DIG{i + 1}_BASE"})
    part(f"Q{1 + i}", "Transistor_BJT:Q_NPN_EBC", "2N2222", TO92,
         {"1": "GND", "2": f"DIG{i + 1}_BASE", "3": f"DIG{i + 1}"})

# ---- Haptic motor (low-side switch + flyback diode).
part("R13", "Device:R", "1k", R_V, {"1": "HAPTIC_DRV", "2": "HAPTIC_BASE"})
part("Q5", "Transistor_BJT:Q_NPN_EBC", "2N2222", TO92, {"1": "GND", "2": "HAPTIC_BASE", "3": "MOTOR_N"})
part("D2", "Device:D", "1N4148", "Diode_THT:D_DO-35_SOD27_P2.54mm_Vertical_AnodeUp", {"1": "+3V3", "2": "MOTOR_N"})
part("J3", "Connector_Generic:Conn_01x02", "MOTOR (+, -)", "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical",
     {"1": "+3V3", "2": "MOTOR_N"})

# ---- RGB LED, 5 mm common cathode, legs in the order R, cathode, G, B.
part("D3", "Device:LED_RKGB", "RGB LED 5mm CC", "LED_THT:LED_D5.0mm-4_RGB",
     {"1": "LED_R_A", "2": "GND", "3": "LED_G_A", "4": "LED_B_A"})
part("R14", "Device:R", "220", R_V, {"1": "LED_R_DRV", "2": "LED_R_A"})
part("R15", "Device:R", "100", R_V, {"1": "LED_G_DRV", "2": "LED_G_A"})
part("R16", "Device:R", "100", R_V, {"1": "LED_B_DRV", "2": "LED_B_A"})

# ---- Shift-register output enable: pulled up so everything is off until the MCU boots.
part("R17", "Device:R", "10k", R_V, {"1": "+3V3", "2": "SR_OE"})

# ---- Buttons (to GND, MCU internal pull-ups).
BTN = "Button_Switch_THT:SW_PUSH_6mm_H13mm"
part("SW1", "Switch:SW_Push", "MODE", BTN, {"1": "BTN_MODE", "2": "GND"})
part("SW2", "Switch:SW_Push", "SET", BTN, {"1": "BTN_SET", "2": "GND"})

# ---- Battery voltage divider.
part("R18", "Device:R", "100k", R_V, {"1": "VBAT", "2": "VBAT_SENSE"})
part("R19", "Device:R", "100k", R_V, {"1": "VBAT_SENSE", "2": "GND"})
part("C3", "Device:C", "100nF", CAP, {"1": "VBAT_SENSE", "2": "GND"})

# ---- Ambient light sensor, powered from a GPIO so it draws nothing in sleep.
part("R21", "Device:R_Photo", "LDR GL5528", "OptoDevice:R_LDR_5.1x4.3mm_P3.4mm_Vertical",
     {"1": "LDR_PWR", "2": "LDR_SENSE"})
part("R20", "Device:R", "10k", R_V, {"1": "LDR_SENSE", "2": "GND"})

# ---- Power. The battery feeds the SuperMini's 5V pin through a Schottky diode.
# Charging has its OWN port: the TP4056 module's USB socket. The module's input
# is deliberately NOT tied to VBUS, so the SuperMini's USB-C is for data only
# and nothing plugged in there can back-feed the charger.
part("D1", "Device:D_Schottky", "1N5819", "Diode_THT:D_DO-41_SOD81_P2.54mm_Vertical_AnodeUp",
     {"1": "VBUS", "2": "VBAT"})
part("J2", "Connector_Generic:Conn_01x03", "TP4056 (GND, OUT+, CHRG)",
     "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical",
     {"1": "GND", "2": "VBAT", "3": "CHRG"})

# ---- Accessory port: a 2x3 socket on the watch face that add-on "packs" plug into.
#   1 GND      2 +3V3
#   3 SDA      4 SCL        I2C bus (shared with the DS3231): sensors, displays
#   5 AUX_PWR  6 AIN        AUX_PWR is 3.3 V that is only on while the watch is awake
#                           (the same GPIO that powers the light sensor, max ~20 mA).
#                           AIN is an audio-rate analogue input: C4 couples it onto
#                           the battery-sense ADC pin, so no extra GPIO is needed.
part("J4", "Connector_Generic:Conn_02x03_Odd_Even", "ACCESSORY 2x3 socket",
     "Connector_PinSocket_2.54mm:PinSocket_2x03_P2.54mm_Vertical",
     {"1": "GND", "2": "+3V3", "3": "SDA", "4": "SCL", "5": "LDR_PWR", "6": "AIN"})
part("C4", "Device:C", "100nF", CAP, {"1": "AIN", "2": "VBAT_SENSE"})

# ---- I2C pull-ups. The bus must idle high even when neither the clock module
# nor a pack (both usually bring their own pull-ups) is fitted.
part("R22", "Device:R", "4.7k", R_V, {"1": "+3V3", "2": "SDA"})
part("R23", "Device:R", "4.7k", R_V, {"1": "+3V3", "2": "SCL"})

# ---- Real-time clock module (wire pads).
part("J1", "Connector_Generic:Conn_01x05", "DS3231 (VCC, SDA, SCL, NC, GND)",
     "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical",
     {"1": "+3V3", "2": "SDA", "3": "SCL", "4": None, "5": "GND"})

for i in range(4):
    part(f"H{i + 1}", "Mechanical:MountingHole", "M2", "MountingHole:MountingHole_2.2mm_M2", {})

POWER_NETS = ["GND", "+3V3", "VBUS", "VBAT"]

SUPERMINI_PINS = {
    "1": "GPIO5", "2": "GPIO6", "3": "GPIO7", "4": "GPIO8", "5": "GPIO9", "6": "GPIO10",
    "7": "GPIO20", "8": "GPIO21", "9": "GPIO0", "10": "GPIO1", "11": "GPIO2", "12": "GPIO3",
    "13": "GPIO4", "14": "3V3", "15": "GND", "16": "5V",
}


def nets():
    out = {}
    for p in PARTS:
        for pin, net in p["pins"].items():
            if net:
                out.setdefault(net, []).append((p["ref"], pin))
    return out


if __name__ == "__main__":
    n = nets()
    for name in sorted(n):
        print(f"{name:12s} " + " ".join(f"{r}.{p}" for r, p in n[name]))
    single = [k for k, v in n.items() if len(v) < 2]
    print("single-pin nets:", single)
