/*
 * Watch V1 - printed accessories
 *
 *   desk_stand   - holds the watch upright on a desk or a music-stand ledge, so a
 *                  class can see the timer / countdown / metronome. The strap stays on.
 *   probe_sleeve - vented case for a sensor board on a cable (the bee kit's hive probe)
 *   probe_plug   - closes the sleeve's open end round the cable
 *
 * Export:  openscad -D 'part="desk_stand"' -o stl/desk_stand.stl accessories.scad
 * No supports needed: print the stand lying on one of its side cheeks, the sleeve
 * standing on its closed end, the plug flat. PETG or PLA, 0.2 mm layers.
 */

part = "all";   // [all, desk_stand, probe_sleeve, probe_plug]

/* [Watch - must match enclosure.scad] */
watch = [63.0, 49.0, 25.3];   // outer x (long), y (short, strap direction), height
lug = [31, 6.2, 3.5];         // strap lug width, how far it sticks out, height (at the base)
strap = [24, 2.5];            // strap width + clearance, thickness

/* [Desk stand] */
tilt = 70;                    // degrees from horizontal
stand_w = 44;                 // width of the stand (the watch overhangs it on both sides)
fit = 0.6;                    // clearance round the watch
ledge = 4;                    // how far the front lip comes up the watch face

/* [Probe sleeve: sensor board on a cable] */
board = [12, 16, 4];          // breakout board incl. parts (GY-BME280 is about 11.5 x 15.5)
sleeve_wall = 1.2;
cable_d = 4.0;                // 4-core cable
vent_w = 1.2;                 // slot width: small enough that bees cannot get in

$fn = 48;

// ----------------------------------------------------------------- desk stand
// The watch lies with its base against a tilted back and its lower long edge in a
// trough. The lower strap lug and the strap hang down through a slot in the trough.
module desk_stand() {
    t = watch[2] + 2 * fit;        // trough width = watch thickness
    back = 3;
    depth = 60;                    // footprint front-to-back
    lift = 3 + lug[1] + 2;         // raises the trough so the lower lug clears the base plate
    cheek = 6;                     // the braces are two side cheeks, leaving the middle free for the strap
    module cradle() { translate([0, 0, lift]) rotate([90 - tilt, 0, 0]) children(); }
    difference() {
        union() {
            translate([-stand_w / 2, -depth / 2, 0]) cube([stand_w, depth, 3]);              // base plate
            cradle() translate([-stand_w / 2, 0, 0]) {
                translate([0, -back, -back]) cube([stand_w, back, 40 + back]);              // back
                translate([0, -back, -back]) cube([stand_w, t + 2 * back, back]);           // trough floor
                translate([0, t, -back]) cube([stand_w, back, ledge + back]);               // front lip
            }
            for (x = [-stand_w / 2, stand_w / 2 - cheek]) hull() {                          // side cheeks
                translate([x, -depth / 2, 0]) cube([cheek, depth / 2, 3]);
                cradle() translate([x, -back, -back]) cube([cheek, back, 40 + back]);     // behind the back only
            }
        }
        cradle() {
            // the lower lug drops through the trough floor
            translate([-(lug[0] + 2) / 2, -back - 1, -back - 1]) cube([lug[0] + 2, back + lug[2] + fit + 1, back + 2]);
            // window in the back for a one-piece strap that runs behind the case
            translate([-strap[0] / 2, -back - 1, -back - 1]) cube([strap[0], back + 1 + strap[1], 46]);
        }
        translate([-100, -100, -50]) cube([200, 200, 50]);                                 // nothing below the table
    }
}

// ----------------------------------------------------------------- hive probe
// A box with slotted sides round the sensor board. Slide the board in, cable last,
// then press the plug in round the cable.
inside = [board[0] + 1, board[2] + 1.5, board[1] + 2];       // x, y, z (board stands on end)
outside = [inside[0] + 2 * sleeve_wall, inside[1] + 2 * sleeve_wall, inside[2] + sleeve_wall];

module probe_sleeve() {
    difference() {
        translate([-outside[0] / 2, -outside[1] / 2, 0]) cube(outside);
        translate([-inside[0] / 2, -inside[1] / 2, sleeve_wall]) cube([inside[0], inside[1], inside[2] + 1]);
        // vents on the two wide faces
        for (s = [-1, 1], k = [0 : 3])
            translate([-inside[0] / 2 + 1.5 + k * (inside[0] - 3 - vent_w) / 3, s * outside[1] / 2 - 2, sleeve_wall + 2])
                cube([vent_w, 4, inside[2] - 5]);
        // and in the closed end
        for (k = [-1, 1]) translate([k * 3 - vent_w / 2, -inside[1] / 2 + 0.5, -1]) cube([vent_w, inside[1] - 1, sleeve_wall + 2]);
    }
}

module probe_plug() {
    plug_h = 5;
    difference() {
        union() {
            translate([-outside[0] / 2, -outside[1] / 2, 0]) cube([outside[0], outside[1], sleeve_wall]);
            translate([-(inside[0] - 0.3) / 2, -(inside[1] - 0.3) / 2, 0]) cube([inside[0] - 0.3, inside[1] - 0.3, plug_h]);
        }
        translate([0, 0, -1]) cylinder(d = cable_d + 0.4, h = plug_h + 2);
        // split so it can go round a cable that is already soldered
        translate([-0.4, 0, -1]) cube([0.8, outside[1], plug_h + 2]);
    }
}

if (part == "desk_stand") desk_stand();
else if (part == "probe_sleeve") probe_sleeve();
else if (part == "probe_plug") probe_plug();
else {
    desk_stand();
    translate([50, 0, 0]) probe_sleeve();
    translate([70, 0, 0]) probe_plug();
}
