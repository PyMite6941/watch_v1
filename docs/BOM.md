# Parts list

Quantities are for **one watch**. Everything is through-hole and hand-solderable. The reference designators (R1, U2…) match the schematic and the board's printing; the authority is [`hardware/scripts/design.py`](../hardware/scripts/design.py) — if this list and that file ever disagree, the file is right.

Buy a few spares of the cheap parts: resistors and transistors come in strips anyway.

## On the board

| Ref | Qty | Part | Notes |
|---|---|---|---|
| U1 | 1 | ESP32-C3 SuperMini | The processor board, USB-C. Soldered to the **back** of the PCB, USB-C towards the right edge. |
| U2, U3 | 2 | 74HC595 shift register, DIP-16 | Must be the **HC** family (works at 3.3 V). Back of the board. |
| DS1 | 1 | 0.56" 4-digit 7-segment display, **common cathode**, 12 pins | 5643AS / 5641AS / CC56-12. Check "common cathode" — the common-anode twin (5643BS) will not light. |
| Q1–Q5 | 5 | NPN transistor, TO-92, pins E-B-C | PN2222A / 2N2222A (TO-92) / 2N3904. Check the pin order of what you buy: flat side facing you, left to right, must be emitter, base, collector. BC547 is C-B-E and would go in backwards. |
| D1 | 1 | 1N5819 Schottky diode | Battery → processor board |
| D2 | 1 | 1N4148 diode | Protects Q5 from the motor |
| D3 | 1 | 5 mm RGB LED, **common cathode**, 4 legs | Leg order red, cathode (longest), green, blue |
| R1–R8, R14 | 9 | 220 Ω resistor, ¼ W | Display segments, red LED |
| R9–R13 | 5 | 1 kΩ | Transistor bases |
| R15, R16 | 2 | 100 Ω | Green and blue LED |
| R17, R20 | 2 | 10 kΩ | Display-off-at-boot pull-up; light sensor divider |
| R18, R19 | 2 | 100 kΩ | Battery measurement |
| R22, R23 | 2 | 4.7 kΩ | I²C pull-ups |
| R21 | 1 | Photoresistor (LDR) GL5528 | Light sensor |
| C1–C4 | 4 | 100 nF ceramic capacitor, 2.5 mm pitch | |
| SW1, SW2 | 2 | 6 × 6 mm tactile switch, **13 mm tall** | The height matters: the stem has to reach through the lid |
| J4 | 1 | 2×3 female pin socket, 2.54 mm, 8.5 mm tall | Accessory socket |
| — | 1 | Watch V1 PCB, 58 × 44 mm | See [ORDERING.md](ORDERING.md) |

Resistors stand upright (one leg folded over) to save space.

J1, J2 and J3 on the board are rows of solder pads for wires, not connectors to buy.

## Off the board, wired to it

| Qty | Part | Connects to | Notes |
|---|---|---|---|
| 1 | Li-Po cell, 3.7 V, about 150 mAh, about 23 × 15 × 5.5 mm, **with protection circuit** | Charger board B+ / B− | e.g. size "551521". Measure before printing the case — the pocket is 23.4 × 16 mm. |
| 1 | TP4056 charger board **with protection** (the kind with six pads: IN+, IN−, B+, B−, OUT+, OUT−) | J2: GND, BAT, CHG | Its own USB socket is the watch's charging port. USB-C boards are about 28 × 17.5 mm, micro-USB ones 26 × 17 mm — set `charger` in the enclosure file to match. |
| 1 | 12 kΩ resistor, **0805 surface-mount** (or see note) | Replaces the charger board's R<sub>PROG</sub> | **Required.** The board ships set for 1000 mA, far too much for a 150 mAh cell. 12 kΩ gives about 100 mA. |
| 1 | DS3231 real-time-clock module, small type ("DS3231 for Raspberry Pi", about 14 × 14 mm) | J1: 3V3, SDA, SCL, GND | Optional but recommended: without it the watch must re-sync over Wi-Fi every 3 hours to stay accurate. |
| 1 | Coin vibration motor, 10 mm, 3 V | J3: +, − | |
| — | Thin stranded wire | | |

About the charge-current resistor: on the TP4056 board, find the small resistor marked `122` (1.2 kΩ) next to the chip, remove it and fit 12 kΩ (`123`). Charge current = 1200 ÷ kΩ, in mA: 12 kΩ → 100 mA, 15 kΩ → 80 mA. Charging a small cell at the board's default rate overheats it — do not skip this.

## Case and strap

| Qty | Part | Notes |
|---|---|---|
| 1 | Printed base and lid | Clear PETG or clear resin — [ORDERING.md](ORDERING.md) |
| 1 | Printed LED dome and LED spacer | `led_dome.stl` (clear or translucent white) and `led_spacer.stl` (any colour); a few grams together |
| 4 | M2 × 16 mm pan-head screws | Self-tap into the lid; or M2 heat-set inserts and `screw_tap_d = 3.2` |
| 1 | 22 mm one-piece strap (NATO or hook-and-loop) | Threads through both lugs |

## Packs (optional, per teacher)

| Pack | Parts | For |
|---|---|---|
| Environment | BME280 3.3 V I²C breakout, 2×3 male header, perfboard scrap | Temperature, humidity, pressure, logging |
| Sound | MAX4466 microphone amplifier board, 2×3 male header, perfboard scrap | Tuner |
| Screen | SSD1306 I²C OLED (0.91" 128×32 is the default) on a Screen Pack board with a 4-pin 2.54 mm female header and a 2×3 male header; or a Qwiic OLED on a Pack Base's Qwiic socket | The current screen in words and pictures: full date, weather, messages, teacher-designed screens ([OLED_SCREENS.md](OLED_SCREENS.md)) |

Wiring is in [ADDONS.md](ADDONS.md). What each pack costs: [COSTS.md](COSTS.md).

## Tools and consumables

Soldering iron with a fine tip, solder, flush cutters, tweezers, multimeter, USB-C **data** cable, small Phillips screwdriver, a little double-sided foam tape for the battery and charger board.

## Prototyping without the PCB

The whole circuit fits on a breadboard using the same parts and the pin table at the top of [`firmware/watch_v1/config.h`](../firmware/watch_v1/config.h). It is worth building one on a breadboard first to prove the firmware before the boards arrive.
