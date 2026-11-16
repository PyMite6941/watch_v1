# Build guide

From a bag of parts to a working watch. Read it through once before starting; allow an evening for the first one.

> **Honest status.** No watch has been built from revision 5 yet. The steps below are the plan; the first build is also the test of this guide. Note anything that did not match and fix it here.

## 0. Before ordering anything

1. Get the board: the Gerbers (`hardware/fab/watch_v1_gerbers.zip`) are made in the cloud — nothing to route yourself ([`hardware/README.md`](../hardware/README.md)). Ordering: [`ORDERING.md`](ORDERING.md).
2. Build the circuit on a breadboard and run the firmware on it (sections 2 and 5 work the same on a breadboard). This proves the design before money is spent on boards.
3. Order boards, cases and parts — [ORDERING.md](ORDERING.md), [BOM.md](BOM.md).

## 1. Prepare the charger board

Do this first; it is the one fiddly surface-mount job.

1. Replace the charge-current resistor on the TP4056 board with 12 kΩ ([BOM.md](BOM.md) explains which one and why).
2. Solder the battery leads to **B+** and **B−**. Red to B+. Check twice: reversing a lithium cell destroys the board and can start a fire.
3. Plug a USB charger into the board: its "charging" light comes on, then the "full" light when done. With a multimeter, OUT+ to OUT− reads 3.7–4.2 V.

## 2. Flash and test the processor board on its own

Easier to do before it is soldered down.

1. Install [Arduino IDE 2](https://www.arduino.cc/en/software). In *Boards Manager* install **esp32 by Espressif Systems** (version 3.x). In *Library Manager* install **ArduinoJson** (version 7), and **Adafruit SSD1306** (it asks to add **Adafruit GFX** too: say yes) for the optional OLED screen pack.
2. Open `firmware/watch_v1/watch_v1.ino`.
3. *Tools* menu:
   - Board: **ESP32C3 Dev Module**
   - USB CDC On Boot: **Enabled**
   - Partition Scheme: **Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)** — the default scheme is too small for Wi-Fi plus Bluetooth
   - Port: the SuperMini's port
4. Click *Upload*. If it will not connect: hold the board's **BOOT** button, tap **RST**, release BOOT, upload again.
5. Open the Serial Monitor at 115200. You should see a line starting `Watch V1 fw 2.2.0`. Type `{"cmd":"info"}` and send; a line of JSON comes back. The processor and firmware are alive.

Command-line alternative:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,PartitionScheme=min_spiffs firmware/watch_v1
arduino-cli upload  --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,PartitionScheme=min_spiffs -p COM5 firmware/watch_v1
```

## 3. Solder the board

The display sits on the front; the processor board and both chips hang from the back, *underneath* the display. That forces the order: **everything under the display first, display last.**

Work lowest parts first so the board lies flat.

| Step | Side | Parts | Watch out for |
|---|---|---|---|
| 1 | Back | U2, U3 (74HC595) | Notch matches the notch in the printed outline; the corner numbers 1, 8, 9, 16 printed beside the pads must match the chip (pin 1 is next to the notch). Solder directly, no sockets — there is no height for them. Trim the legs flush on the front; the display sits over them. |
| 2 | Back | U1 (SuperMini) | Components and USB socket facing **away** from the PCB, USB-C towards the right edge ("USB-C THIS END"). The board prints each pad's name (5V, GND, 3V3, 4, 3 …): check they match the names printed on the SuperMini before soldering. Use the pin headers that came with it, plastic spacer against the PCB, and trim flush on the front. |
| 3 | Front | Resistors R1–R23 | Standing up. Values are in [BOM.md](BOM.md); read the colour bands or measure each one. |
| 4 | Front | Diodes D1, D2 | The band (cathode) matches the printed band. |
| 5 | Front | Capacitors C1–C4 | Either way round. |
| 6 | Front | Transistors Q1–Q5 | Flat side matches the outline (flat faces the bottom edge). `EBC` is printed under each one: emitter, base, collector, left to right. |
| 7 | Front | LDR R21, RGB LED D3 | `RKGB` is printed under the LED: red, cathode, green, blue, left to right — the longest leg goes in the second hole (K). Thread the legs through the printed **LED spacer** (`led_spacer.stl`, 3 mm) and push the LED down onto it, standing straight up: that puts its tip exactly where the lid's light dome expects it. Leave the spacer in. Stand the LDR about 8 mm tall. Keep R14 and R17 (either side of the LED) no taller than 9 mm, because the dome's ring passes just above them. R11 and R12 stand about 0.5 mm from the LED's rim: solder them straight up, not leaning towards it. |
| 8 | Front | Switches SW1, SW2 | They click in. |
| 9 | Front | J4 accessory socket | Hold it square while tacking one pin. Pin 1 is the square pad, with a `1` printed beside it. |
| 10 | — | **Stop and test** (section 4) | |
| 11 | Front | DS1 display | Decimal points at the bottom, on the edge printed `DP THIS EDGE`; the corner pins are numbered 1, 6, 7, 12 on the board. Sit it on a 2 mm spacer (two matchsticks) so it clears the joints beneath, solder two opposite corners, check it is level, then the rest. Once it is in, the parts under it cannot be reworked. |

Wires:

| Pads | To |
|---|---|
| J2 `G` / `BAT` / `CHG` | Charger board OUT− / OUT+ / the pad of its *charging* LED nearest the chip (the TP4056's CHRG pin). CHG is optional: without it the watch simply never shows the charging dot. |
| J1 `3V3` / `SDA` / `SCL` / `GND` | DS3231 module (leave `NC` empty) |
| J3 `+` / `−` | Vibration motor |

## 4. Test before the display goes in

With the battery **not** connected, power from the SuperMini's USB-C.

1. Multimeter: 3.3 V between the `3V3` and `G` pads of J1.
2. Press each button: the serial monitor is silent, but the motor should tick (once it is wired).
3. Send `{"cmd":"buzz","ms":500}` — the motor runs for half a second.
4. Send `{"cmd":"info"}` — `"rtc":true` if the clock module is wired.
5. Hold the display in its holes at a slight angle so the pins touch (do not solder yet): digits should appear. Dim or missing segments mean a resistor value or a solder joint on that segment; a missing digit means that digit's transistor.

Then solder the display, connect the charger board, and unplug USB: the watch should wake from the battery when a button is pressed.

## 5. Set it up

[CONFIGURATION.md](CONFIGURATION.md): open Watch Studio, connect by USB, go through Quick setup, choose the screens for this teacher, save. Press *Sync time over Wi-Fi*; the watch shows `SYnC` then `On`.

## 6. Case

1. Print `fit_test.stl` first (ten minutes). Check that an M2 screw passes the post, bites in the boss, and that the PCB corner drops onto the post. Adjust `screw_tap_d`, `gap` in `enclosure/enclosure.scad` if not, and re-export.
2. Print `led_dome.stl` and `led_spacer.stl` (both stand on their flat side; print the dome in clear or translucent white, 0.12–0.16 mm layers so the facets come out crisp). Print `base.stl` as it is and `lid.stl` as it is (already flipped so the outer face is on the bed). For a personal case, use a patterned lid (`lid_honeycomb.stl`, `lid_binary.stl`, `lid_staff.stl`, `lid_sine.stl`) or set *Personal touches* in `enclosure.scad` (lid pattern, text under the base) and export your own; see [`kits/`](../kits/README.md). 0.2 mm layers, 3 walls, no supports. In clear PETG, print slow and hot with the fan low for the best transparency.
3. Charger board on the floor between its two rails, USB socket in the left-hand opening; a square of foam tape holds it. Battery in its pocket to the right.
4. Lower the PCB onto the four posts, wires tucked into the space under the display — not across the battery.
5. Push the LED dome up through the hexagonal hole from the inside of the lid, so its ring lies flat against the lid's underside (it only fits one way round: the ring's straight edge faces the display). Lid on: the LED slides up into the dome's bore; buttons, light sensor and accessory socket line up with their openings. The lid clamps the ring, so the dome cannot fall out or turn. Hold SET on a clock screen for party mode: the dome changes colour twice a second.
6. Four M2 × 16 screws from underneath. Snug only; the plastic strips easily.
7. Thread the strap through both lugs.

## 7. Final check

- [ ] Wakes on each button, sleeps after a few seconds
- [ ] Time is right after a sync; still right the next morning
- [ ] Display dims when the light sensor is covered
- [ ] Vibrates on presses
- [ ] Charges from the left socket (charger light visible through the case) and runs with nothing plugged in
- [ ] Watch Studio connects over USB (right socket) and over Bluetooth (both buttons, 2 s)
- [ ] Each pack this teacher is getting works: sensor screens show numbers; the tuner follows a hummed note

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Nothing at all on battery, fine on USB | D1 backwards, or the charger board's protection has tripped — plug the charger in for a second to reset it |
| Whole display dark, motor works | R17 pulling the display off is normal until the firmware starts; if it stays dark, check the wire from GPIO20 to pin 13 of both chips |
| One segment missing on every digit | That segment's resistor (R1–R8) or a joint on U2 |
| One digit missing | That digit's transistor (Q1–Q4) or its 1 kΩ resistor |
| Digits ghost into each other | A transistor in backwards (check E-B-C) |
| Time resets | DS3231 not detected (`"rtc":false`) — check its four wires and its coin cell |
| Battery screen reads far off | R18 / R19 values swapped with something else; both must be 100 kΩ |
| Will not upload | Hold BOOT, tap RST, release BOOT; use a data cable, not a charge-only one |
| Uploads, then "Sketch too big" | Partition Scheme is not set to Minimal SPIFFS |
| Watch seems dead after a bad setting | Connect by USB and send `{"cmd":"factory_reset"}` |
