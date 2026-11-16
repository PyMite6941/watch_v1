# Hardware

The circuit is described once, in code, and everything else is generated from it.

```
scripts/design.py      THE circuit: every part, its value, footprint and which net each pin is on
scripts/gen_sch.py     -> kicad/watch_v1.kicad_sch   (any Python 3)
scripts/gen_pcb.py     -> kicad/watch_v1.kicad_pcb   (KiCad's own Python: needs the pcbnew module)
scripts/check_wiring.py  audits the generated schematic and board against datasheet pinouts
scripts/freeroute.py   -> kicad/watch_v1.ses         (Freerouting; run for you by the cloud workflow)
scripts/route.py       -> kicad/watch_v1.routes.json (any Python 3; experimental autorouter)
scripts/export_fab.py  -> fab/watch_v1_gerbers.zip   (checks the board first)
kicad/                 the generated KiCad 10 project - open watch_v1.kicad_pro
addons/                add-on packs: the Pack Base board (routed, design-rule clean) and the pack plan
```

**Edit `design.py`, not the KiCad files** — anything changed by hand in the schematic or in the footprint positions is overwritten the next time the scripts run. Tracks are the exception: see below.

## Current state — revision 5

| Check | Result |
|---|---|
| Schematic electrical rules (ERC) | 0 findings |
| Placement self-checks (display orientation, USB end, parts inside the outline) | all pass |
| Wiring audit (`check_wiring.py`) | 55 checks against datasheet pinouts and the firmware's pin table, all pass |
| Board against schematic | 0 differences |
| Design rules (DRC) | clean apart from routing, which the cloud workflow does (see *Routing*) |

The board is placed; routing and the Gerbers come from the cloud workflow below, so nobody has to run a router.

## Finding pin 1 — on the drawings and on the parts

Every part that can go in the wrong way round is marked in three places: a note on the schematic next to its block, printing on the board, and (for the accessory socket) the case.

| Part | On the real part | On the board | Counting |
|---|---|---|---|
| U1 SuperMini | USB-C socket at one end; pad names printed on it | Outline with the USB end drawn and "USB-C THIS END"; each pad's name printed (5V, GND, 3V3, 4, 3, 2, 1, 0 / 5, 6, 7, 8, 9, 10, 20, 21); pad 1 is square | Component side up, USB at the top: pads 1–8 down the left, 9–16 up the right. Pad 1 = GPIO5, pad 16 = 5V. |
| U2, U3 74HC595 | Notch (and often a dot) at the pin 1 end | Notch in the outline; corner numbers **1, 8, 9, 16** printed beside the pads; pad 1 square | Notch up: 1–8 down the left, 9–16 up the right |
| DS1 display | Decimal points along the bottom edge | "DP THIS EDGE"; corner pins **1, 6, 7, 12** printed | From the front: 1–6 left to right along the bottom, 7–12 right to left along the top |
| Q1–Q5 | Flat face | Outline with the flat; **EBC** printed under each | Flat towards you, legs down: E, B, C |
| D3 RGB LED | Longest leg = cathode | **RKGB** printed under it | Left to right: red, cathode, green, blue |
| D1, D2 | Band = cathode | "K" at the cathode pad | |
| J4 accessory socket | — | **1** printed beside pin 1; square pad on the back | Top row 1 GND, 3 SDA, 5 AUX; bottom row 2 3V3, 4 SCL, 6 AIN. The lid opening has a round notch at the pin 1 corner. |
| J1, J2, J3 wire pads | — | Every pad named: 3V3 SDA SCL NC GND / G BAT CHG / MOTOR + − | |

Parts on the back of the board (U1, U2, U3) are drawn mirrored in KiCad's front view, as they must be; flip the view (`View → Flip Board View`) to see them as you will when soldering.

```
   U2 / U3  (74HC595)                  DS1 from the front               Q1-Q5, flat face to you
      ┌──╮╭──┐                    12  11  10   9   8   7                    ┌─────────┐
  QB 1│  ╰╯  │16 VCC             ┌──────────────────────┐                   │         │
  QC 2│      │15 QA              │  8.   8.   8.   8.   │                   └─┬──┬──┬─┘
  QD 3│      │14 SER             └──────────────────────┘                     E  B  C
  QE 4│      │13 /OE               1   2   3   4   5   6
  QF 5│      │12 RCLK                                                   J4 from the front
  QG 6│      │11 SRCLK           D3, legs down                          ╭ notch
  QH 7│      │10 /SRCLR           R  K  G  B                            ● 1 GND  3 SDA  5 AUX
 GND 8│      │9  QH'                 └ longest                            2 3V3  4 SCL  6 AIN
      └──────┘
```

## Reading the schematic

The sheet is drawn as an annotated netlist: every pin has a short stub and a label, and **pins with the same label are connected**. There are no long wires to follow. Each block has a title (MCU, segment driver, display, power…).

## The circuit in one paragraph

A Li-Po cell feeds the SuperMini's 5 V pin through Schottky diode D1; the SuperMini's own regulator makes 3.3 V for everything. The cell is charged by a separate TP4056 board through *its* USB socket, which is deliberately not connected to the SuperMini's USB — that port stays free for data. Two 74HC595 shift registers turn four processor pins into sixteen outputs: U2 drives the eight display segments through 220 Ω resistors; U3 switches the four digits (through NPN transistors Q1–Q4), the three colours of the RGB LED, and the vibration motor (through Q5). The shift registers' output-enable pin is driven with PWM for brightness and pulled high by R17 so nothing lights before the firmware starts. A DS3231 keeps time on the I²C bus, which also goes to accessory socket J4. R18/R19 halve the battery voltage for measurement; C4 couples the accessory socket's audio input onto that same pin. The light sensor divider (R21/R20) and the socket's AUX pin are powered from a processor pin so they cost nothing in sleep.

| GPIO | Net | Purpose |
|---|---|---|
| 0 | VBAT_SENSE | Battery ÷ 2 (ADC); accessory audio rides on it through C4 |
| 1 | LDR_SENSE | Light sensor (ADC) |
| 2 | — | Strapping pin, left unconnected on purpose |
| 3, 4 | BTN_MODE, BTN_SET | Buttons to ground; both can wake the chip from deep sleep |
| 5 | CHRG | Charger "charging" signal, low = charging |
| 6, 7, 10 | SR_DATA, SR_CLK, SR_LATCH | Shift registers |
| 8, 9 | SDA, SCL | I²C: clock chip, accessory socket; pulled up by R22, R23 |
| 20 | SR_OE | Shift-register output enable (PWM brightness) |
| 21 | LDR_PWR | Switched 3.3 V: light sensor and accessory AUX pin |

Every pin the SuperMini exposes is used. That is why the accessory socket shares pins rather than getting its own.

## Regenerating

Windows paths shown; the KiCad Python lives inside the KiCad install.

```bash
cd hardware/scripts
python gen_sch.py
"%LOCALAPPDATA%\Programs\KiCad\10.0\bin\python.exe" gen_pcb.py
python check_wiring.py
```

`check_wiring.py` is the independent audit. It ignores `design.py`, takes the netlist KiCad extracts from the schematic and the pad nets of the board, and checks them against pinouts written out from the datasheets and against `firmware/watch_v1/config.h` — for example "segment A: U2 output QA (pin 15) → R1 → display pin 11". Run it after every change; it ends with `ALL CHECKS PASSED` or names what is wrong.

What it cannot check: that the parts you buy match those datasheets. The three that vary between sellers are the display (must be common cathode with the 5641AS/5643AS pinout), the transistors (must be E-B-C) and the RGB LED (must be common cathode, leg order R-K-G-B). Test each with a multimeter's diode range before soldering.

Set `KICAD_SYMBOL_DIR` / `KICAD_FOOTPRINT_DIR` if KiCad is installed somewhere else. `gen_pcb.py` prints a line per placement check and fails if one does not pass.

To move a part, change its entry in `PLACE` in `gen_pcb.py` (millimetres from the board's top-left corner, looking at the display side). The enclosure reads the same coordinates — if the display, buttons, LED, light sensor, accessory socket or USB end move, update the matching numbers at the top of `enclosure/enclosure.scad`.

## Routing — done in the cloud

**Nobody building a watch has to route anything or install Freerouting or Java.** The board is routed by a GitHub workflow, [`.github/workflows/route-board.yml`](../.github/workflows/route-board.yml), on GitHub's computers:

1. `gen_pcb.py` places the parts (unrouted).
2. `freeroute.py` runs [Freerouting](https://github.com/freerouting/freerouting) 2.5.0 headless (checksum-verified download) and saves `kicad/watch_v1.ses`.
3. `gen_pcb.py` places again, this time importing the tracks.
4. `check_wiring.py` audits the wiring, and `export_fab.py` runs the design-rule check and writes `fab/watch_v1_gerbers.zip`.
5. The routed board and the Gerbers are pushed to the branch `routed-board`, and kept as a download on the run (*routed-board*).

It runs by itself whenever `design.py`, `gen_pcb.py` or the footprints change on `main`, and by hand from **GitHub › Actions › Route the board › Run workflow**. If you only want to order boards, take `hardware/fab/watch_v1_gerbers.zip` and go to [`docs/ORDERING.md`](../docs/ORDERING.md).

`gen_pcb.py` imports `watch_v1.ses` only when it was made for the current placement (it records a fingerprint of the pads in `watch_v1.ses.json`), so a stale route is never applied to a moved part.

### Other ways, for experimenting

- **Freerouting on your own computer:** `set FREEROUTING_JAR=…freerouting-2.5.0.jar`, then run `freeroute.py` and `gen_pcb.py` with KiCad's Python. It wants about 1 GB of free memory (`FREEROUTING_MEM`).
- **By hand in KiCad:** open `kicad/watch_v1.kicad_pcb`; `X` draws a track, `V` drops a via. Net classes are set (signals 0.3 mm, power 0.4 mm, 0.2 mm clearance). Hand-drawn tracks are lost if `gen_pcb.py` runs again.
- **`route.py`**, the small in-repo maze router (any Python 3, about 400 MB). Experimental: it has never finished this board; its output (`watch_v1.routes.json`) is used only when there is no Freerouting session.

## Exporting for manufacture

```bash
python export_fab.py
```

Runs the design-rule check and stops with a list if anything real is wrong; otherwise writes `fab/watch_v1_gerbers.zip`. It ignores four kinds of cosmetic finding that are expected here (through-hole parts sharing the area under the display, and printing clipped at pads). Ordering advice is in [`docs/ORDERING.md`](../docs/ORDERING.md).

Before sending files, open the zip in KiCad's Gerber viewer or the factory's online preview and look at it: outline closed, four corner holes, printing readable.

## Things worth knowing about this board

- **Parts on both sides.** The display is on the front; the SuperMini and both chips are on the back, directly under it. Solder the back parts first and trim their legs — see the build guide.
- **Antenna.** The SuperMini's chip antenna ends up near the middle of the board under the display. The router is told to keep tracks away from it, but range may still suffer; it is on the project test list.
- **GPIO2 is unused deliberately.** It decides the boot mode at power-up; anything connected to it can stop the watch starting.
- **GPIO8 and 9** are the I²C bus and also the SuperMini's on-board LED and BOOT button. The LED will flicker with I²C traffic; that is normal.
- **Charging does not go through the SuperMini.** Plugging a computer into the right-hand USB-C powers the watch (D1 blocks that power from reaching the battery) but only the charger board's socket charges.
