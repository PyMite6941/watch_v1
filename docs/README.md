# ESP32-C3 SuperMini Watch (Watch V1)

A cyber‑punk style, see‑through wrist watch built around an ESP32‑C3 SuperMini, a 4‑digit 7‑segment display, two tactile buttons, ambient‑light sensor (LDR) for auto‑brightness, and a rechargeable Li‑Po battery charged via USB‑C.

The watch is designed to be 3D‑printed in clear PETG/resin so the internal circuitry is visible, with a snap‑fit or screw‑together enclosure that holds the PCB, battery, and display.

## Repository layout
- `docs/` – Documentation (this README, build guide, license).
- `src/` – Source files: firmware, schematic, parts list, KiCad project.
- `assets/` – 3D models (OpenSCAD) and other assets for the enclosure.

## Getting started
1. Review the [Build Guide](docs/BUILD_GUIDE.md) for wiring and assembly instructions.
2. Order the parts listed in `src/parts_list.md`.
3. Wire the breadboard per `src/ascii_schematic.txt`.
4. Flash the firmware (`src/firmware.ino`) to the ESP32‑C3 SuperMini.
5. Test the display, buttons, and auto‑brightness.
6. When satisfied, generate Gerbers from the KiCad project (`src/watch_v1.kicad_proj`) and order a PCB, or assemble on perfboard.
7. Print the enclosure from the OpenSCAD files in `assets/`.
8. Assemble the printed case, insert the PCB and battery, and seal with optional conformal coating.

## License
MIT License – see `docs/LICENSE`.

## Contributions
Feel free to fork, improve, and submit pull requests. This project is intended as an open‑source hardware gift for teachers and fellow makers.
