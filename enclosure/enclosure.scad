/*
 * Watch V1 enclosure - parametric OpenSCAD model (works in OpenSCAD 2021.01+).
 *
 * Two printed parts, no supports needed:
 *   base - open box with four PCB posts, two USB openings and strap lugs. Print as modelled.
 *          Right-hand wall: the SuperMini's USB-C (data and firmware).
 *          Left-hand wall:  the TP4056 charger board's own USB socket (charging only).
 *   lid  - top plate with display window, button/LED/LDR holes, the accessory-socket opening and four bosses
 *          that clamp the PCB. Print UPSIDE DOWN (outer face on the bed) - the
 *          "plate" part already has it flipped.
 *
 * Four M2 x 16 screws go in from the bottom, through the base posts and the
 * PCB, and self-tap into the lid bosses. Nothing is visible on the top face.
 *
 * Export from the command line:
 *   openscad -D "part=\"base\"" -o stl/base.stl enclosure.scad
 *   openscad -D "part=\"lid\""  -o stl/lid.stl  enclosure.scad
 * or in the GUI: set `part` below, press F6, then File > Export > STL.
 *
 * A personal case: set "Personal touches" below, e.g.
 *   openscad -D "part=\"lid\"" -D "lid_pattern=\"honeycomb\"" -o stl/lid_honeycomb.stl enclosure.scad
 *   openscad -D "part=\"base\"" -D "base_text=\"THANK YOU\"" -o my_base.stl enclosure.scad
 * Ready-made setups for particular people are in ../kits/. Stand and probe case: accessories.scad.
 *
 * All PCB positions are in the same coordinates as hardware/scripts/gen_pcb.py:
 * millimetres from the board's top-left corner, looking at the display side.
 */

part = "assembly";   // "base", "lid", "plate" (both, ready to print), "assembly", "fit_test",
                     // "led_dome", "led_spacer", "led_parts" (both LED pieces, ready to print)

/* [PCB - must match the KiCad board] */
pcb = [58, 44, 1.6];
pcb_holes = [[3, 3], [55, 3], [3, 41], [55, 41]];
display_center = [29, 15.5];
display_size = [50.3, 19.0];
button_centers = [[7.75, 34.65], [50.25, 34.65]];
led_center = [29, 29.5];
ldr_center = [14.5, 27.7];
usb_center_y = 15.62;          // centre of the SuperMini on the right-hand edge
accessory_center = [45.74, 2.7];   // J4, the 2x3 accessory socket
accessory_opening = [8.6, 6.2];    // 0 width = no opening. Tall enough to join the display window: no thin sliver between them
accessory_pin1_notch_d = 2.0;      // round notch marking the socket's pin 1 corner

/* [Heights] */
below_pcb = 10.0;              // room under the PCB: SuperMini + USB-C, battery, charger (which sits partly under U3)
above_pcb = 10.5;              // room above the PCB: display on a 2 mm standoff
usb_below_pcb = 5.1;           // USB-C centre line below the PCB underside

/* [Shell] */
wall = 2.0;
floor_t = 1.6;
lid_t = 1.6;
gap = 0.5;                     // clearance between PCB edge and wall (PCBs are cut to +-0.2 mm)
corner_r = 3.0;
lip_h = 1.5;                   // lid lip that drops inside the base
lip_t = 1.2;
lip_gap = 0.3;                 // lid lip to wall, each side: slides in without sanding on a typical FDM printer

/* [Openings] */
window_margin = 0.3;
window_membrane = 0.4;         // 0 = open hole; 0.4 = two clear layers over the display
button_hole_d = 4.2;           // 6x6x13 mm tactile switch stem is 3.5 mm
ldr_hole_d = 5.5;              // 0 = no hole (fine with a clear lid)

/* [RGB LED light dome: a separate small print, part "led_dome"] */
// A hollow half-hexagon dome over the 5 mm RGB LED. Its hexagonal stem passes up
// through a hexagonal hole in the lid with a gap all the way round; a full ring
// (the collar) under the lid joins it to the lid all the way round the circle and is
// clamped there, so it cannot fall out or turn. The LED stands inside the stem's
// round bore with clearance on every side, and lights the whole dome.
led_d = 5.0;                   // LED body
led_bore_gap = 0.3;            // LED to bore, each side
dome_wall = 0.6;               // stem and dome wall (a 0.4 mm nozzle prints it as 1.5 lines)
dome_fit = 0.2;                // stem to lid hole, each side
dome_h = 2.6;                  // how far the dome stands above the lid
dome_top_flats = 3.6;          // across the flats of the dome's flat top
dome_top_t = 1.0;              // light-spreading thickness over the LED
collar_t = 0.8;
collar_d = 11.0;               // the ring under the lid; trimmed straight on the display side
bore_chamfer = 0.6;            // lead-in at the bore's mouth that guides the LED in
led_tip_above_pcb = 11.6;      // where the LED's tip ends up (set by the LED spacer)
led_body_h = 8.6;              // a standard 5 mm LED: flange to tip
usb_cutout = [13.0, 7.5];      // wide enough for a USB-C plug's plastic body

/* [Charger board: TP4056 module lying on the floor against the left-hand wall] */
// MEASURE YOUR MODULE. USB-C versions are about 28 x 17.5 mm, micro-USB ones 26 x 17.
charger = [28, 17.5];          // length (along x, from the left wall), width; length 0 = no charger port
charger_center_y = 29.5;       // PCB y of the module's centre line = centre of its USB socket
charger_usb_z = 2.8;           // socket centre above the floor: board thickness + half the socket height
charger_fence_h = 2.5;

/* [Screws: M2 x 16 pan head] */
screw_clear_d = 2.4;
screw_tap_d = 1.8;             // self-tap into the lid boss; use 3.2 for M2 heat-set inserts
screw_head_d = 4.4;
screw_head_h = 2.5;
post_d = 5.5;
boss_d = 5.0;

/* [Strap] */
strap_w = 22;                  // strap width; a one-piece NATO / velcro strap threads through both lugs
strap_slot = 3.2;
lug_bar = 3.0;
lug_h = 3.5;

/* [Battery fence on the floor] */
battery = [23.4, 16, 3];       // length, width, fence height; set height 0 to remove
battery_corner = [28.6, 25.5]; // top-left corner in PCB coordinates (just right of the charger board)

/* [Personal touches] */
// Engraved into the lid's outer face, in the strip below the display (clear of every hole).
lid_pattern = "none";          // [none, honeycomb, binary, staff, sine]
lid_binary_text = "CS";        // for "binary": each character's 8 bits as long (1) / short (0) bars
pattern_depth = 0.4;           // 2 layers at 0.2 mm
// Engraved under the base, readable when the watch is turned over. Empty = none.
base_text = "";
base_text2 = "";
base_text_size = 4.5;
base_text_depth = 0.5;

$fn = 32;

// ---------------------------------------------------------------- derived
inner = [pcb[0] + 2 * gap, pcb[1] + 2 * gap];
outer = [inner[0] + 2 * wall, inner[1] + 2 * wall];
z_pcb = floor_t + below_pcb;               // underside of the PCB
z_wall = z_pcb + pcb[2] + above_pcb;       // top of the base walls = underside of the lid plate
total_h = z_wall + lid_t;
echo(str("outer size: ", outer[0], " x ", outer[1], " x ", total_h, " mm"));

// PCB coordinates (y down, origin top-left) -> model coordinates (y up)
function at(p) = [wall + gap + p[0], wall + gap + pcb[1] - p[1]];

// LED dome, derived. Hexagons are described across their flats; hex(f) is the
// circumradius OpenSCAD wants for a 6-sided cylinder with flats f apart.
function hex(f) = f / (2 * cos(30));
led_bore_d = led_d + 2 * led_bore_gap;
stem_flats = led_bore_d + 2 * dome_wall;
led_hole_flats = stem_flats + 2 * dome_fit;
led_spacer_h = led_tip_above_pcb - led_body_h;
led_ring_d = 2 * hex(led_hole_flats) + 2.4;  // keep-out round the LED for lid patterns
// From the LED's centre to the display window's edge: the lid between them holds the
// collar, so the collar is trimmed 0.2 mm short of it.
led_to_window = led_center[1] - (display_center[1] + display_size[1] / 2 + window_margin);
collar_trim = led_to_window - 0.2;
echo(str("LED: spacer ", led_spacer_h, " mm, lid hole ", led_hole_flats, " mm across flats, ",
         "lid wall to the window ", led_to_window - led_hole_flats / 2, " mm, collar rim on the display side ",
         collar_trim - led_hole_flats / 2, " mm"));
assert(led_to_window - led_hole_flats / 2 >= 0.5, "LED hole too close to the display window");
assert(collar_trim - led_hole_flats / 2 >= 0.3, "collar has no rim on the display side");
assert(collar_d / 2 - hex(led_hole_flats) >= 0.8, "collar too small to hold at the hole's corners");
assert(led_tip_above_pcb > above_pcb - collar_t, "the LED tip must reach into the dome's bore");
assert(led_tip_above_pcb + 0.6 < above_pcb + lid_t + dome_h - dome_top_t, "the LED would touch the top of the dome");
assert(led_spacer_h >= 0, "LED spacer would be negative: lower led_tip_above_pcb");

module rounded_box(size, r) {
    hull() for (x = [r, size[0] - r], y = [r, size[1] - r])
        translate([x, y, 0]) cylinder(r = r, h = size[2]);
}

module strap_lug() {   // sits at z = 0 against a long wall, pointing in +y
    w = strap_w + 1 + 2 * 4;
    difference() {
        translate([-w / 2, -0.01, 0]) cube([w, strap_slot + lug_bar, lug_h]);
        translate([-(strap_w + 1) / 2, -0.02, -1]) cube([strap_w + 1, strap_slot, lug_h + 2]);
    }
}

module base() {
    difference() {
        union() {
            rounded_box([outer[0], outer[1], z_wall], corner_r);
            translate([outer[0] / 2, outer[1], 0]) strap_lug();
            translate([outer[0] / 2, 0, 0]) mirror([0, 1, 0]) strap_lug();
        }
        // cavity
        translate([wall, wall, floor_t]) cube([inner[0], inner[1], z_wall]);
        // USB-C opening in the right-hand wall
        translate([outer[0] - wall - 1, at([0, usb_center_y])[1] - usb_cutout[0] / 2,
                   z_pcb - usb_below_pcb - usb_cutout[1] / 2])
            cube([wall + 2, usb_cutout[0], usb_cutout[1]]);
        // charger USB opening in the left-hand wall
        if (charger[0] > 0)
            translate([-1, at([0, charger_center_y])[1] - usb_cutout[0] / 2,
                       max(0.6, floor_t + charger_usb_z - usb_cutout[1] / 2)])
                cube([wall + 2, usb_cutout[0], usb_cutout[1]]);
        if (base_text != "") translate([0, 0, -1]) linear_extrude(base_text_depth + 1) base_text_2d();
    }
    // two rails that keep the charger board square to its opening
    if (charger[0] > 0) for (side = [-1, 1])
        translate([wall, at([0, charger_center_y])[1] + side * (charger[1] / 2 + 0.3) - (side < 0 ? 1 : 0), floor_t])
            translate([4, 0, 0]) cube([charger[0] - 8, 1, charger_fence_h]);
    // PCB posts with screw clearance and a counterbore for the head
    for (h = pcb_holes) translate(concat(at(h), [0])) difference() {
        cylinder(d = post_d, h = z_pcb);
        translate([0, 0, -1]) cylinder(d = screw_clear_d, h = z_pcb + 2);
        translate([0, 0, -1]) cylinder(d = screw_head_d, h = screw_head_h + 1);
    }
    // battery fence
    if (battery[2] > 0) translate(concat(at([battery_corner[0], battery_corner[1] + battery[1]]), [floor_t]))
        difference() {
            translate([-1, -1, 0]) cube([battery[0] + 2, battery[1] + 2, battery[2]]);
            translate([0, 0, -1]) cube([battery[0], battery[1], battery[2] + 2]);
            translate([battery[0] / 2 - 4, -2, -1]) cube([8, battery[1] + 4, battery[2] + 2]);  // wire exits
        }
}

// Lid in its assembled position (plate on top of the walls).
module lid() {
    difference() {
        union() {
            translate([0, 0, z_wall]) rounded_box([outer[0], outer[1], lid_t], corner_r);
            // locating lip
            translate([wall + lip_gap, wall + lip_gap, z_wall - lip_h]) difference() {
                cube([inner[0] - 2 * lip_gap, inner[1] - 2 * lip_gap, lip_h]);
                translate([lip_t, lip_t, -1])
                    cube([inner[0] - 2 * lip_gap - 2 * lip_t, inner[1] - 2 * lip_gap - 2 * lip_t, lip_h + 2]);
            }
            // bosses that press the PCB onto the posts
            for (h = pcb_holes) translate(concat(at(h), [z_pcb + pcb[2]]))
                cylinder(d = boss_d, h = above_pcb);
        }
        for (h = pcb_holes) translate(concat(at(h), [z_pcb + pcb[2] - 1]))
            cylinder(d = screw_tap_d, h = above_pcb - 1);
        // display window (leaves a thin membrane on the outer face if asked to)
        translate([at(display_center)[0] - display_size[0] / 2 - window_margin,
                   at(display_center)[1] - display_size[1] / 2 - window_margin, z_wall - lip_h - 1])
            cube([display_size[0] + 2 * window_margin, display_size[1] + 2 * window_margin,
                  lip_h + 1 + lid_t - window_membrane + (window_membrane > 0 ? 0 : 1)]);
        for (b = button_centers) translate(concat(at(b), [z_wall - 1])) cylinder(d = button_hole_d, h = lid_t + 2);
        // hexagonal hole for the LED dome's stem (flats left-right, like the dome)
        translate(concat(at(led_center), [z_wall - 1])) cylinder(r = hex(led_hole_flats), h = lid_t + 2, $fn = 6);
        if (ldr_hole_d > 0) translate(concat(at(ldr_center), [z_wall - 1])) cylinder(d = ldr_hole_d, h = lid_t + 2);
        // accessory socket
        if (accessory_opening[0] > 0) {
            ax = at(accessory_center)[0] - accessory_opening[0] / 2;
            ay = at(accessory_center)[1] - accessory_opening[1] / 2;
            // through the plate
            translate([ax, ay, z_wall - 1]) cube([accessory_opening[0], accessory_opening[1], lid_t + 2]);
            // relief in the lip, which the socket body would otherwise hit
            translate([ax, ay, z_wall - lip_h - 1]) cube([accessory_opening[0], accessory_opening[1] + 3, lip_h + 1]);
            // round notch at the PIN 1 corner (top-left seen from the front), so a pack
            // cannot be read the wrong way round. Pin 1 = GND.
            translate([ax, ay + accessory_opening[1], z_wall - 1]) cylinder(d = accessory_pin1_notch_d, h = lid_t + 2);
        }
        if (lid_pattern != "none")
            translate([0, 0, total_h - pattern_depth]) linear_extrude(pattern_depth + 1) lid_pattern_2d();
    }
}

// ------------------------------------------------------- personal touches
// The strip of lid below the display, in model coordinates, minus a ring round each hole.
band_pcb_y = [display_center[1] + display_size[1] / 2 + window_margin + 1.2, pcb[1] + gap - 0.6];
band_x = [wall + 1.0, outer[0] - wall - 1.0];
band_y = [at([0, band_pcb_y[1]])[1], at([0, band_pcb_y[0]])[1]];

module band_area() {
    difference() {
        translate([band_x[0], band_y[0]]) square([band_x[1] - band_x[0], band_y[1] - band_y[0]]);
        for (b = button_centers) translate(at(b)) circle(d = button_hole_d + 2.4);
        translate(at(led_center)) circle(d = led_ring_d);
        if (ldr_hole_d > 0) translate(at(ldr_center)) circle(d = ldr_hole_d + 2.4);
    }
}

module lid_pattern_2d() {
    w = band_x[1] - band_x[0];
    h = band_y[1] - band_y[0];
    intersection() {
        band_area();
        if (lid_pattern == "honeycomb") {
            s = 3.2;                                           // cell size across flats
            for (i = [-1 : w / (s * 0.866) + 1], j = [-1 : h / s + 1])
                translate([band_x[0] + i * s * 0.866 * 1.0, band_y[0] + j * s + (i % 2) * s / 2])
                    difference() { circle(d = s / 0.866 + 0.01, $fn = 6); circle(d = (s - 0.5) / 0.866, $fn = 6); }
        } else if (lid_pattern == "staff") {
            for (k = [0 : 4]) translate([band_x[0], band_y[0] + h / 2 + (k - 2) * 1.6 - 0.2]) square([w, 0.4]);
        } else if (lid_pattern == "sine") {
            n = 60;
            for (k = [0 : n - 1]) hull() for (q = [k, k + 1])
                translate([band_x[0] + q * w / n, band_y[0] + h / 2 + 0.38 * h * sin(q * 720 / n)]) circle(d = 0.6, $fn = 8);
            translate([band_x[0], band_y[0] + h / 2 - 0.15]) square([w, 0.3]);          // the x-axis
        } else if (lid_pattern == "binary") {
            bits = [for (c = lid_binary_text) for (b = [7 : -1 : 0]) floor(ord(c) / pow(2, b)) % 2];
            pitch = w / max(1, len(bits));
            for (k = [0 : len(bits) - 1])
                translate([band_x[0] + (k + 0.5) * pitch - 0.3, band_y[0] + h / 2 - (bits[k] ? 0.35 : 0.15) * h])
                    square([0.6, (bits[k] ? 0.7 : 0.3) * h]);
        }
    }
}

module base_text_2d() {
    // Seen from underneath, so mirrored in x to read the right way round.
    translate([outer[0] / 2, outer[1] / 2]) mirror([1, 0]) {
        translate([0, base_text2 == "" ? 0 : base_text_size * 0.75])
            text(base_text, size = base_text_size, halign = "center", valign = "center", font = "Liberation Sans:style=Bold");
        if (base_text2 != "") translate([0, -base_text_size * 0.75])
            text(base_text2, size = base_text_size * 0.8, halign = "center", valign = "center", font = "Liberation Sans");
    }
}

// ------------------------------------------------------------ LED dome
// Modelled standing on its collar (z = 0 = underside of the lid), which is also
// how it prints: collar on the bed, no supports. Clear or translucent white
// PETG / resin; a 0.4 mm nozzle; 0.12-0.16 mm layers make the facets crisp.
module led_dome() {
    stem_h = collar_t + lid_t;
    cone_h = dome_h - dome_top_t;
    flats_at = function(z) stem_flats - (stem_flats - dome_top_flats) * z / dome_h;
    difference() {
        union() {
            intersection() {                                                      // the ring under the lid
                cylinder(d = collar_d, h = collar_t, $fn = 64);
                translate([-collar_d, -collar_d, -1]) cube([2 * collar_d, collar_d + collar_trim, collar_t + 2]);
            }
            cylinder(r = hex(stem_flats), h = stem_h + 0.01, $fn = 6);            // through the lid
            translate([0, 0, stem_h])                                             // the half-hexagon dome
                cylinder(r1 = hex(stem_flats), r2 = hex(dome_top_flats), h = dome_h, $fn = 6);
        }
        // the LED's bore, then a cone that keeps the dome's walls dome_wall thick
        translate([0, 0, -1]) cylinder(d = led_bore_d, h = stem_h + 1.01, $fn = 48);
        translate([0, 0, -0.01]) cylinder(d1 = led_bore_d + 2 * bore_chamfer, d2 = led_bore_d, h = bore_chamfer, $fn = 48);
        translate([0, 0, stem_h])
            cylinder(d1 = led_bore_d, d2 = flats_at(cone_h) - 2 * dome_wall, h = cone_h, $fn = 48);
    }
}

// Sets the LED's height while it is soldered: the LED's flange sits on it, the legs
// go through the slot. Leave it in place.
module led_spacer() {
    difference() {
        translate([-3.3, -1.6, 0]) cube([6.6, 3.2, led_spacer_h]);
        translate([-2.4, -0.6, -1]) cube([4.8, 1.2, led_spacer_h + 2]);
    }
}

module lid_for_printing() {
    translate([0, outer[1], total_h]) rotate([180, 0, 0]) lid();
}

// Small piece to check screw, post and boss fit before printing the real thing.
module fit_test() {
    intersection() {
        base();
        translate([-10, -10, 0]) cube([wall + gap + 16, wall + gap + 16, z_pcb + 2]);
    }
    translate([30, 0, 0]) intersection() {
        lid_for_printing();
        translate([-1, -1, 0]) cube([wall + gap + 17, wall + gap + 17, total_h]);
    }
}

if (part == "base") base();
else if (part == "lid") lid_for_printing();
else if (part == "plate") { base(); translate([0, outer[1] + 14, 0]) lid_for_printing(); }
else if (part == "fit_test") fit_test();
else if (part == "led_dome") led_dome();
else if (part == "led_spacer") led_spacer();
else if (part == "led_parts") { led_dome(); translate([collar_d / 2 + 6, 0, 0]) led_spacer(); }
else {
    base();
    %lid();
    translate(concat(at(led_center), [z_wall - collar_t])) color("white", 0.8) led_dome();
    color("green", 0.6) translate([wall + gap, wall + gap, z_pcb]) cube(pcb);
}
