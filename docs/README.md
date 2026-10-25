# ESP32-C3 SuperMini Watch (Watch V1)

A cyber‑punk style, see‑through wrist watch built around an ESP32‑C3 SuperMini, a 4‑digit 7‑segment display, two touch buttons, haptics vibration motor, ambient‑light sensor (LDR) for auto‑brightness, and a rechargeable Li‑Po battery charged via USB‑C or optional wireless charging receiver.

The watch is designed to be 3D‑printed in clear PETG/resin so the internal circuitry is visible, with a snap‑fit or screw‑together enclosure that holds the PCB, battery, and display.

## Repository layout
- `docs/` – Documentation (this README, build guide, license).
- `src/` – Source files: firmware, schematic, parts list, KiCad project.
- `assets/` – 3D models (OpenSCAD) and other assets for the enclosure.
- `docs/NETLIST.csv` – Connection netlist derived from the ASCII schematic, useful for importing into KiCad.

## Getting started
1. Review the [Build Guide](docs/BUILD_GUIDE.md) for wiring and assembly instructions.
2. Order the parts listed in `src/parts_list.md`.
3. Wire the breadboard per `src/ascii_schematic.txt`.
4. Flash the firmware (`src/firmware.ino`) to the ESP32‑C3 SuperMini.
   - The firmware now includes optional Wi‑Fi + NTP time synchronisation (set your SSID/PASS at the top of the file), deep‑sleep power saving, and touch‑button sensing; retains full manual time setting via buttons/encoder.
5. Test the display, touch buttons, and auto‑brightness.
6. When satisfied, generate Gerbers from the KiCad project (`src/watch_v1.kicad_proj`) and order a PCB, or assemble on perfboard.
   - The schematic (`src/watch_v1.kicad_sch`) now contains a detailed connection comment block to assist with wiring.
7. Print the enclosure from the OpenSCAD files in `assets/`.
8. Assemble the printed case, insert the PCB and battery, and seal with optional conformal coating.


## Filament & 3D printing
- **Material:** Clear PETG (or transparent resin) for the enclosure.
- **Estimated filament needed:** ~3.5–4 m of 1.75 mm PETG (≈10 g) for base, lid and optional frame.
- **Recommended spool:** 1 kg PETG filament (1.75 mm) – more than enough for many prints.
- **Where to buy (Lazada Thailand):** https://www.lazada.co.th/products/creality-supply-petg-filament-10kg-175mm-3-i5364792975.html


## Power saving & battery life
- Deep‑sleep with timer wake‑up (1 s) reduces average current to < 20 µA.
- Touch buttons eliminate mechanical pull‑up current.
- Display brightness scaled via LDR and PWM‑like pulse width.
- Peripherals (Wi‑Fi, BLE) powered down when not in use.
- Expected battery life > 60 days on a 150 mAh Li‑Po with typical use.
## Viewing and downloading files
All source files are hosted on GitHub and can be viewed or downloaded directly:
- **Schematic:** https://github.com/pymite6941/watch_v1/blob/main/src/watch_v1.kicad_sch
- **Firmware:** https://github.com/pymite6941/watch_v1/blob/main/src/firmware.ino
- **Parts list:** https://github.com/pymite6941/watch_v1/blob/main/src/parts_list.md
- **ASCII schematic:** https://github.com/pymite6941/watch_v1/blob/main/src/ascii_schematic.txt
- **Enclosure OpenSCAD:** https://github.com/pymite6941/watch_v1/blob/main/assets/enclosure.scad
- **Build guide:** https://github.com/pymite6941/watch_v1/blob/main/docs/BUILD_GUIDE.md
- **Connection netlist (CSV):** https://github.com/pymite6941/watch_v1/blob/main/docs/NETLIST.csv
- **USB‑C notes:** https://github.com/pymite6941/watch_v1/blob/main/docs/USB_C_NOTES.md

To download a file, click the "Raw" button on the file's GitHub page, or use the URL with `?raw=true` appended.
## License
This project is licensed under the MIT License – see `docs/LICENSE`.

In short: you are free to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the software, provided that the above copyright notice and this permission notice are included in all copies or substantial portions of the Software.

Attribution: Please retain the copyright notice (which includes the author's name) in any distribution.

## Contributions
Feel free to fork, improve, and submit pull requests manually.
Automated or bot-generated pull requests are not allowed.
Please ensure your contributions align with the project’s goals and follow the existing code style.

