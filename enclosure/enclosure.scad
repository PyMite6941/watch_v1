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
 * All PCB positions are in the same coordinates as hardware/scripts/gen_pcb.py:
 * millimetres from the board's top-left corner, looking at the display side.
 */

part = "assembly";   // "base", "lid", "plate" (both, ready to print), "assembly", "fit_test"

/* [PCB - must match the KiCad board] */
pcb = [58, 44, 1.6];
pcb_holes = [[3, 3], [55, 3], [3, 41], [55, 41]];
display_center = [29, 15.5];
display_size = [50.3, 19.0];
button_centers = [[7.75, 34.65], [50.25, 34.65]];
led_center = [29, 28.5];
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
gap = 0.4;                     // clearance between PCB edge and wall
corner_r = 3.0;
lip_h = 1.5;                   // lid lip that drops inside the base
lip_t = 1.2;
lip_gap = 0.2;

/* [Openings] */
window_margin = 0.3;
window_membrane = 0.4;         // 0 = open hole; 0.4 = two clear layers over the display
button_hole_d = 4.2;           // 6x6x13 mm tactile switch stem is 3.5 mm
led_hole_d = 5.4;
ldr_hole_d = 5.5;              // 0 = no hole (fine with a clear lid)
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
        translate(concat(at(led_center), [z_wall - 1])) cylinder(d = led_hole_d, h = lid_t + 2);
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
else {
    base();
    %lid();
    color("green", 0.6) translate([wall + gap, wall + gap, z_pcb]) cube(pcb);
}
