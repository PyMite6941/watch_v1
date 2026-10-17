# Tools Used in the ESP32‑C3 SuperMini Watch (Watch V1) Project

This document lists the software and hardware tools used to design, simulate, and build the watch.

## Software Tools
- **KiCad** (version 6 or later) – Electronic Design Automation suite used to create the schematic (`watch_v1.kicad_sch`) and PCB layout. Generates Gerber files for fabrication.
- **OpenSCAD** – Parametric 3D modeling tool used to create the enclosure (`enclosure.scad`). Allows easy adjustments of dimensions and export to STL for 3D printing.
- **Arduino IDE** (with ESP32 board package) – Used to write, compile, and upload the firmware (`firmware.ino`) to the ESP32‑C3 SuperMini.
- **Serial Monitor** (built into Arduino IDE) – For debugging and viewing LDR values, time ticks, etc.
- **GitHub Pages** – Hosts the project documentation from the `docs/` folder, enabling a live website at `https://<username>.github.io/<repo>/`.
- **Markdown** – Used for README, build guide, and license files.
- **HTML/CSS** – Simple landing page (`index.html`) for the documentation site.
- **Shell scripts** – `export_ascii.sh` to display the ASCII schematic over SSH (uses `img2txt` or `catimg` if available, otherwise falls back to `cat`).
- **ImageMagick** (`convert`) – Used to generate a placeholder PNG schematic (`schematic.png`) from the ASCII art (optional).
- **Blender / MeshLab** (optional) – For inspecting or repairing STL files before printing.

## Hardware Tools (for assembly & testing)
- **Soldering iron & thin solder** – For making permanent connections on perfboard or PCB.
- **Wire strippers & tweezers** – To prepare jumper wires and place small components.
- **Heat‑shrink tubing or electrical tape** – To insulate solder joints.
- **Multimeter** – To verify continuity, check resistor values, diode orientation, and battery voltage.
- **Breadboard & jumper wires** – For prototyping the circuit before soldering.
- **USB‑C cable** – For powering the ESP32‑C3 SuperMini and charging the Li‑Po battery via the TP4056 module.
- **3D printer** (FDM or SLA) – To print the enclosure in clear PETG/resin (or any material). Recommended layer height: 0.1‑0.2 mm, 20 % infill.
- **M2×6 mm screws** (or printed snap‑fit posts) – To secure the PCB to the base and attach the lid.
- **Isopropyl alcohol & conformal coating spray** (optional) – To clean the PCB and add a moisture‑resistant layer.
- **Double‑sided foam tape or adhesive** – To hold the 5643A‑1 display in place without obstructing light.

## Libraries & Dependencies (firmware)
- **Arduino core for ESP32** – Provides `Arduino.h`, `BLEDevice.h`, etc.
- **No external libraries required** beyond the core; the firmware uses only built‑in functions.

## References
- KiCad documentation: https://docs.kicad.org/
- OpenSCAD manual: https://www.openscad.org/doc/
- ESP32 Arduino core: https://github.com/espressif/arduino-esp32
- GitHub Pages guide: https://docs.github.com/pages
