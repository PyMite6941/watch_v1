# 3D Enclosure Plan for Watch V1

## Overview
The enclosure consists of three printable parts:
1. **Base** – bottom part with battery pocket and screw posts.
2. **Lid** – top cover with display window and LED holder.
3. **Frame** (optional) – internal snap‑fit ring to hold the PCB.

All parts are parameterized in `enclosure.scad`.

## Parameters to Adjust
- `outer_length`, `outer_width`, `outer_height`: overall dimensions (default 60 × 40 × 20 mm).
- `wall_thickness`: thickness of walls (default 2 mm).
- `lid_overlap`: how much the lid overlaps the base (default 1.5 mm).
- `display_window`: [width, height] of the 7‑segment display cutout (default [12, 8] mm).
- `battery_pocket`: [length, width, depth] of the Li‑Po holder (default [30, 15, 5] mm).
- `screw_hole_diameter`: clearance for M2 screws (default 2.2 mm).
- `screw_hole_spacing`: [X, Y] distance from origin to screw hole centers (default [56, 36] mm).
- LED holder parameters (for top‑right corner):
  - `led_holder_size`: diameter across points of hexagonal holder (default 8 mm).
  - `led_holder_height`: height of holder (default 3 mm).
  - `led_holder_led_diameter`: diameter for LED hole (default 5.2 mm for a 5 mm LED).
  - `led_holder_post_diameter`: diameter of snap post (default 1.8 mm).
  - `tolerance`: interference for snap fit (default 0.2 mm).
  - `led_holder_outward_offset`: how much holder sticks out beyond lid corner (default 2 mm).

## USB‑C Access
A USB‑C cutout is already present in the `base()` module (sidewall, front, centered). 
You can adjust its position and size by modifying the following parameters at the top of the file:
- `usb_cutout_width` (mm) – width of the cutout (default 8)
- `usb_cutout_height` (mm) – height of the cutout (default 4)
- `usb_cutout_offset_x` (mm) – X offset from center (positive right) (default 0)
- `usb_cutout_offset_y` (mm) – Y offset from center (positive toward front edge) (default 0)
The cutout is applied as a horizontal slot through the side wall (width = wall_thickness) via the translation in `base()`:
```
translate([(outer_length - usb_cutout_width)/2, 0, 2]) cube([usb_cutout_width, wall_thickness, usb_cutout_height]);
```
Adjust the `usb_cutout_offset_x`/`usb_cutout_offset_y` values or the Z translation (currently 2 mm) to move the cutout as needed.

## Generating STL Files
1. Open `enclosure.scad` in OpenSCAD.
2. Press **F6** to render.
3. To export a part, uncomment exactly one of the lines at the bottom of the file:
   - `base();`   // export bottom/base
   - `lid();`    // export lid/top
   - `frame();`  // export internal frame (optional)
4. Press **F6** again, then **File → Export → Export as STL**.
5. Repeat for each part, changing the uncommented line each time.

## Printing Tips
- Print in clear PETG or transparent resin at 0.1‑0.2 mm layer height.
- Use 20 % infill (or 100 % for the window area if you want it solid but clear).
- After printing, clean any support material.
- Test the snap fit of the LED holder and adjust `tolerance` if needed (print the `test_snap()` module if uncommented).

## Assembly
1. Insert the PCB into the base; align screw holes with posts.
2. Secure the PCB with M2×6 mm screws (or use printed posts as snap‑fits).
3. Place the Li‑Po battery into the battery pocket.
4. Lay the display face‑up in the window.
5. Place the lid over the base; align screw holes and fasten.
6. Snap the LED holder onto the post on the lid top‑right corner.
7. Insert the RGB LED into the holder’s central hole.
8. Optional: apply conformal coating to the PCB and seal the seam with silicone.

## License
This design is released under the MIT License – see `docs/LICENSE`.