# Assets – 3D Models and CAD files

This directory contains the OpenSCAD source files for the watch enclosure.

## Files
- `enclosure.scad` – Parametric OpenSCAD model of the watch box, battery pocket, screw holes, and display window.
  - To generate STL files, open the file in OpenSCAD, uncomment the desired module (`base();`, `lid();`, or `frame();`) at the bottom, press **F6** to render, then **File → Export → Export as STL**.

## Printing tips
- Use clear PETG or transparent resin for the top lid and side walls to showcase the internal circuitry.
- The bottom base can be printed in any color; white or black provides good contrast for the PCB.
- Recommended layer height: 0.1‑0.2 mm, 20 % infill (or 100 % for structural parts).
- After printing, clean support threads and verify that the screw holes are clear (use a 2.2 mm drill bit if needed).

## Assembly
Refer to `docs/BUILD_GUIDE.md` for detailed steps on inserting the PCB, battery, and securing the lid with M2 screws.

## License
The models are released under the MIT License – see the top‑level `docs/LICENSE`.
