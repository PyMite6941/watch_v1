# Build Guide – ESP32‑C3 SuperMini Watch (Watch V1)

All source files (schematic, firmware, parts list, enclosure design, etc.) are available for viewing and download on GitHub: https://github.com/pymite6941/watch_v1

## 1. Parts procurement
Refer to `src/parts_list.md` for the full bill of materials, including the recommended YDL 3.7 V 150 mAh Li‑Po battery (JST‑PH 2.0 mm), TP4056 charger, Schottky diode, 3.3 V LDO, touch buttons (or optional tactile buttons), resistors, LDR, RGB LED, enclosure materials, and an optional wireless charging receiver.

## 2. Breadboard prototype
1. Lay out the breadboard.
2. Place the ESP32‑C3 SuperMini (USB‑C facing outward for easy programming).
3. Insert the 5643A‑1 4‑digit display.
4. Wire exactly as shown in `src/ascii_schematic.txt`:
   - Segment pins (a‑g, dp) → 220 Ω resistors → ESP32 GPIOs 7‑14.
   - Digit commons → collector of NPN transistors (2N2222); emitter → GND; base → ESP32 GPIOs 0,2,4,5 via 4.7 kΩ.
   - Buttons → connected to GPIOs 1 (MODE) and 3 (LED) (touch sensing, no external pull‑up needed).
   - LDR → one side to 3.3 V, other side to GPIO6 with a 10 kΩ pull‑down to GND.
   - RGB LED → connect each anode (R, G, B) through a 220 Ω resistor to ESP32 GPIOs 16, 17, 18; common cathode to GND.
   - Haptics: Connect vibration motor + to collector of NPN transistor (2N2222); emitter to GND. Motor - to +3.3V via flyback diode (cathode to +3.3V, anode to motor -). Base of transistor through 1kΩ resistor to ESP32 GPIO15.
   - Power: USB‑C VBUS → TP4056 IN → BAT+ / BAT‑ to Li‑Po → Schottky diode (cathode to VBUS side) → 3.3 V LDO input → LDO output → ESP32 VIN and 3.3 V rail.
   - Add decoupling capacitors (1 µF) near LDO input/output if available.
5. Double-check all connections, especially the Schottky diode orientation (cathode to VBUS side) and transistor polarity (NPN: collector to digit common, emitter to GND).

## 3. Firmware upload

1. Install the ESP32 board package in the Arduino IDE (Boards Manager → esp32 by Espressif Systems).
2. Select **ESP32C3 Dev Module** (or ESP32‑C3 SuperMini if available) and the correct USB‑C port.
3. Open `src/firmware.ino`.
4. At the top of the file, set your Wi‑Fi SSID and password (leave empty to skip NTP sync). The firmware will attempt to connect and synchronise time via NTP; if credentials are not provided, it will fall back to the internal millis() clock.
5. Optional: add `Serial.println` statements in `updateBrightness()` to debug LDR values.
6. Click **Upload**. After upload, open the Serial Monitor at 115200 baud.
7. You should see the time incrementing each second.
8. Touch BTN1 to cycle through HH:MM, MM:SS, MM:DD.
9. Touch BTN2 to cycle through LED colors (including OFF) with a short press, or to toggle party mode with a long press.
10. Hold BTN1 to fast‑adjust the time (adds 5 minutes per hold).
11. Cover/expose the LDR (e.g., with your finger) – the display should dim in darkness and brighten in light.
12. The watch enters light sleep after 3 seconds of inactivity; pressing any button wakes it instantly.

## 4. PCB fabrication (optional but recommended)
1. Open the KiCad project `assets/watch_v1/watch_v1.kicad_proj`.
2. Verify the schematic matches the ASCII diagram; add any missing nets.
   - You can import the connection netlist from `docs/NETLIST.csv` to help with wiring.
3. Assign footprints (use standard through‑hole footprints for all components; ESP32‑C3 SuperMini has a predefined footprint).
4. Run **Electrical Rules Check (ERC)** and fix any warnings.
5. Switch to the PCB editor, set board outline to 60 mm × 40 mm (Edge.Cuts layer).
6. Place components, keeping the display centered on the top side, buttons on one side, LDR near the edge for light exposure.
7. Route traces (0.25 mm width for signals, 0.5 mm for power). Keep segment lines short and symmetric.
8. Add four 2.2 mm drill holes for M2 screws in the corners (matching the enclosure design).
9. Run **Design Rules Check (DRC)** and fix errors.
10. Generate Gerber files (File → Plot → Format: Gerber, select all layers).
11. Send the Gerbers to a PCB fab (e.g., JLCPCB, PCBWay) for a 2‑layer board.

## 5. 3D‑printed enclosure

**USB‑C access:** A USB‑C cutout is already present in `assets/enclosure.scad` (base sidewall, front, centered). Adjust its position if needed by modifying the `usb_cutout_offset_x`/`usb_cutout_offset_y` variables or the Z translation in the `base()` module.
The enclosure consists of three printable parts:
- **Top cover (lid)** – clear panel with a window for the display and an integrated hexagonal LED holder on the top‑right corner.
- **Bottom base** – houses the battery holder and PCB mounting posts.
- **Internal frame** (optional) – snaps around the PCB to hold it securely.

All parts are parameterized in OpenSCAD (`assets/enclosure.scad`). To generate STL files:

1. Install OpenSCAD (https://www.openscad.org).
2. Open `assets/enclosure.scad`.
3. Adjust the following variables at the top of the file if needed:
   - `outer_length = 60;` // mm
   - `outer_width = 40;`
   - `outer_height = 20;`
   - `wall_thickness = 2;`
   - `display_window = [12, 8];` // width, height of the display cutout
   - `battery_pocket = [30, 15, 5];` // length, width, depth
   - `screw_hole_diameter = 2.2;` // for M2 screws
   - `screw_hole_spacing = [56, 36];` // positions from origin
   - `lid_overlap = 1.5;` // mm overlap of lid over base
   - // LED holder parameters (top‑right corner)
   - `led_holder_size = 8;` // diameter across points of hexagonal holder (mm)
   - `led_holder_height = 3;` // height of holder (mm)
   - `led_holder_led_diameter = 3;` // diameter for LED hole (mm)
   - `led_holder_post_diameter = 1.8;` // diameter of snap post (mm)
   - `tolerance = 0.2;` // interference for snap fit (hole diameter = post - tolerance)
   - `led_holder_post_height = 1.5;` // depth of blind hole in holder / height of post (mm)
   - `led_holder_outward_offset = 2;` // how much holder sticks out beyond lid corner (mm)
4. Press **F6** to render, then **File → Export → Export as STL**.
5. Export three STL files: `top.stl`, `bottom.stl`, `frame.stl` (or however you split the model; the script includes modules for each).
6. **Optional snap‑fit test:** Before printing the full lid, you can uncomment the `test_snap()` module at the bottom of the file, press F6, and export a small test piece to verify that the post and hole interlock with the desired snapping force on your printer. Adjust `tolerance` if needed (increase for tighter fit, decrease for looser).
7. Print in clear PETG or transparent resin at 0.1‑0.2 mm layer height, 20 % infill (or 100 % for the window area if you want it solid but clear; you can leave the window as a hole and attach a clear sheet later).
8. After printing, clean any support material.

## Filament estimation
- **Material:** Clear PETG (or transparent resin).
- **Estimated volume:** ~7.8 cm³ for base + lid + optional frame.
- **Density of PETG:** ~1.25 g/cm³ → mass ≈ 9.8 g.
- **Filament diameter:** 1.75 mm → cross‑section ≈ 2.405 mm².
- **Length needed:** ~3.2 m; add ~10 % margin → **≈ 3.5–4 m** of filament.
- **Where to buy:** A 1 kg spool (e.g., Creality Supply PETG on Lazada Thailand) is more than enough: https://www.lazada.co.th/products/creality-supply-petg-filament-10kg-175mm-3-i5364792975.html

## 6. Assembly
1. Insert the PCB into the bottom base; align the screw holes with the posts.
2. Secure the PCB with M2×6 mm screws (or use the printed posts as snap‑fits if designed).
3. Place the Li‑Po battery into the battery pocket (the pocket holds the battery snugly; add a small piece of foam if needed).
4. If using the wireless charging receiver, place the coil module inside the enclosure (e.g., adhered to the inner side of the top cover or base) and connect its VOUT to TP4056 IN+ and GND to TP4056 IN‑ (respecting polarity). Ensure the coil aligns with a wireless charger pad when charging.
5. Lay the display face‑up in the window; if you printed a separate clear window, glue it with a tiny amount of cyanoacrylate or use a snap‑fit lip.
6. Place the top cover over the bottom base; align the screw holes and fasten with the remaining M2 screws.
7. **Snap the LED holder:** The lid includes a small post on its top‑right corner. The hexagonal LED holder (printed as part of the lid) has a blind hole that fits over this post. Align the holder with the post and press down until it clicks into place.
8. Insert the RGB LED into the central hole of the LED holder; the LED should sit flush with the holder’s top surface.
9. Optional: apply a thin conformal coating to the PCB (avoid covering the display and buttons) and seal the seam between top and bottom with a silicone bead for splash resistance.
10. Optional: place a small dome/diffuser over the RGB LED to spread light evenly, especially useful in party mode.


## 7. Final testing
Power the watch via USB‑C; it should run from USB and charge the battery.
Unplug USB; the watch should continue running from the battery.
Verify the auto‑brightness still works (the LDR must be exposed to ambient light through a small hole or the clear top).
Enjoy your see‑through cyber‑punk watch!

## 8. Troubleshooting
- **Display dim or flickering** – check segment resistor values (220 Ω is correct for 3.3 V); ensure transistors are fully turning on (measure voltage across digit common when enabled).
- **Buttons not responding** – confirm internal pull‑up is enabled in firmware (`INPUT_PULLUP`); check wiring to GND.
- **LDR not changing brightness** – verify the voltage divider: measure GPIO6 voltage with a multimeter while covering/exposing the LDR; should vary between ~0 V and ~3.3 V.
- **Battery not charging** – ensure TP4056 PROG resistor is correctly soldered; check USB‑C voltage at TP4056 IN (~5 V); measure voltage across BAT+ and BAT‑ while charging (should rise to ~4.2 V).
- **RGB LED not lighting** – verify the 220 Ω resistors are in series with each color channel and that the common cathode is connected to GND; check firmware for LED control (short/long press on BTN2).
- **LED holder not snapping** – ensure the post on the lid and the blind hole in the holder are free of debris; press firmly until you feel a click. If the holder is loose, check that the post diameter matches the hole (`led_holder_post_diameter` minus `tolerance`) and that the holder is not warped. Consider printing a snap‑fit test piece (see step 6 in the enclosure section) to fine‑tune `tolerance` for your printer.
- **LED not seated in holder** – verify the LED’s diameter matches the hole (3 mm) and that the LED’s leads are not obstructing insertion.


## Power saving & battery life
- The firmware uses deep‑sleep with timer wake‑up (1 s) to achieve < 20 µA average current.
- Touch buttons eliminate pull‑up current.
- Display brightness is scaled via LDR and PWM‑like pulse width.
- Wi‑Fi and BLE are powered down when not in use.
- Expected battery life > 60 days on a 150 mAh Li‑Po with typical use.
## License
This documentation is released under the MIT License – see `docs/LICENSE`.

In short: you are free to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the documentation, provided that the above copyright notice and this permission notice are included in all copies or substantial portions.

Attribution: Please retain the copyright notice (which includes the author's name) in any distribution.

## Contributions
Feel free to fork, improve, and submit pull requests manually.
Automated or bot-generated pull requests are not allowed.
Please ensure your contributions align with the project’s goals and follow the existing style.

