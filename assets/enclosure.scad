/*
 * Enclosure for ESP32‑C3 SuperMini Watch (Watch V1)
 * OpenSCAD parametric model.
 * 
 * Parameters (adjust at the top):
 *   outer_length, outer_width, outer_height – external dimensions of the box.
 *   wall_thickness – thickness of the walls.
 *   display_window – [width, height] of the cutout for the 7‑segment display.
 *   battery_pocket – [length, width, depth] of the Li‑Po holder.
 *   screw_hole_diameter – diameter for M2 clearance holes.
 *   screw_hole_spacing – [x, y] spacing of the four screw holes from the origin (center).
 *   lid_overlap – how much the lid overlaps the base (for snap‑fit or screw).
 *   // LED holder parameters (for top‑right corner)
 *   led_holder_size – diameter across points of hexagonal holder (mm)
 *   led_holder_height – height of holder (mm)
 *   led_holder_led_diameter – diameter for LED hole (mm)
 *   led_holder_post_diameter – diameter of snap post (mm)
 *   led_holder_hole_diameter – diameter of blind hole in holder (mm) = post - tolerance
 *   led_holder_post_height – depth of blind hole in holder / height of post (mm)
 *   led_holder_outward_offset – how much holder sticks out beyond lid corner (mm)
 *   tolerance – interference for snap fit (positive means hole smaller than post)
 * 
 * To generate STL files:
 *   - Uncomment the module you want to export at the bottom of the file.
 *   - Press F6 to render, then File → Export → Export as STL.
 */

/*///////////////////////
// PARAMETERS //
/////////////////////*/
outer_length = 60;   // mm (X axis)
outer_width  = 40;   // mm (Y axis)
outer_height = 20;   // mm (Z axis)

wall_thickness = 2;          // mm
lid_overlap    = 1.5;        // mm overlap of lid over base

display_window = [12, 8];    // width, height of display cutout (centered on top)
battery_pocket = [30, 15, 5];// length, width, depth of battery holder (centered on bottom inside)

screw_hole_diameter = 2.2;   // clearance for M2 screws
screw_hole_spacing  = [56, 36]; // X and Y distance from origin to hole centers (origin at model center)

// LED holder parameters (for top‑right corner)
led_holder_size = 8;          // diameter across points of hexagonal holder (mm)
led_holder_height = 3;        // height of holder (mm)
led_holder_led_diameter = 5.2;  // diameter for LED hole (mm) – fits 5 mm LED
led_holder_post_diameter = 1.8; // diameter of snap post (mm)
tolerance = 0.2;              // interference: hole diameter = post - tolerance
led_holder_hole_diameter = led_holder_post_diameter - tolerance;
led_holder_post_height = 1.5;   // depth of blind hole in holder / height of post (mm)
led_holder_outward_offset = 2;  // how much holder sticks out beyond lid corner (mm)

/*///////////////////////
// DERIVED VALUES //
/////////////////////*/
half_l = outer_length/2;
half_w = outer_width/2;
half_h = outer_height/2;

inner_length = outer_length - 2*wall_thickness;
inner_width  = outer_width  - 2*wall_thickness;
inner_height = outer_height - wall_thickness - lid_overlap; // height of base (without lid)

base_height = outer_height - lid_overlap;

/*///////////////////////
// MODULES //
/////////////////////*/
// Module: base (bottom part with battery pocket and screw posts)
module base() {
    difference() {
        // Outer block
        cube([outer_length, outer_width, base_height]);
        
        // Inner cavity (empty space for PCB)
        translate([wall_thickness, wall_thickness, 0])
            cube([inner_length, inner_width, base_height]);
        
        // Battery pocket (cutout from inner bottom)
        translate([
            wall_thickness + (inner_length - battery_pocket[0])/2,
            wall_thickness + (inner_width  - battery_pocket[1])/2,
            0
        ])
            cube([battery_pocket[0], battery_pocket[1], battery_pocket[2]]);
        
        // Screw holes (through base and lid)
        for (x = [-1, 1])
            for (y = [-1, 1])
                translate([
                    x * (screw_hole_spacing[0]/2),
                    y * (screw_hole_spacing[1]/2),
                    -wall_thickness // go a bit below to ensure full cut
                ])
                    cylinder(d=screw_hole_diameter, h=base_height+2*wall_thickness, center=false);
    }
}

// Module: lid (top cover with display window and LED holder)
module lid() {
    union() {
        difference() {
            // Outer block (lid thickness)
            cube([outer_length, outer_width, lid_overlap]);
            
            // Inner cavity (matching base inner dimensions)
            translate([wall_thickness, wall_thickness, 0])
                cube([inner_length, inner_width, lid_overlap]);
            
            // Display window cutout
            translate([
                wall_thickness + (inner_length - display_window[0])/2,
                wall_thickness + (inner_width  - display_window[1])/2,
                0
            ])
                cube([display_window[0], display_window[1], lid_overlap+0.1]); // extra to cut through
            
            // Screw holes (align with base)
            for (x = [-1, 1])
                for (y = [-1, 1])
                    translate([
                        x * (screw_hole_spacing[0]/2),
                        y * (screw_hole_spacing[1]/2),
                        -wall_thickness
                    ])
                        cylinder(d=screw_hole_diameter, h=lid_overlap+2*wall_thickness, center=false);
        }
        // LED holder post (on lid top surface)
        translate([
            outer_length - led_holder_size/2 + led_holder_outward_offset,
            outer_width  - led_holder_size/2 + led_holder_outward_offset,
            lid_overlap
        ])
            cylinder(d=led_holder_post_diameter, h=led_holder_post_height);
        
        // LED holder (hexagonal prism with holes)
        difference() {
            // holder base
            translate([
                outer_length - led_holder_size/2 + led_holder_outward_offset,
                outer_width  - led_holder_size/2 + led_holder_outward_offset,
                lid_overlap
            ])
                cylinder(d=led_holder_size, h=led_holder_height, $fn=6);
            // LED hole (through holder)
            translate([
                outer_length - led_holder_size/2 + led_holder_outward_offset,
                outer_width  - led_holder_size/2 + led_holder_outward_offset,
                lid_overlap
            ])
                cylinder(d=led_holder_led_diameter, h=led_holder_height+0.1, center=true);
            // Snap hole (blind hole for post) - slightly smaller for interference fit
            translate([
                outer_length - led_holder_size/2 + led_holder_outward_offset,
                outer_width  - led_holder_size/2 + led_holder_outward_offset,
                lid_overlap - led_holder_post_height
            ])
                cylinder(d=led_holder_hole_diameter, h=led_holder_post_height);
        }
    }
}

// Optional: internal frame that snaps around PCB (simple version)
module frame() {
    // A thin wall that protrudes from the base inner walls to hold PCB
    // We'll model as a "lip" of 0.5mm height on the inner top surface.
    difference() {
        // Base inner block raised slightly
        translate([wall_thickness, wall_thickness, base_height-0.5])
            cube([inner_length, inner_width, 0.5]);
        
        // Cutout for battery pocket (so frame does not interfere)
        translate([
            wall_thickness + (inner_length - battery_pocket[0])/2,
            wall_thickness + (inner_width  - battery_pocket[1])/2,
            base_height-0.5
        ])
            cube([battery_pocket[0], battery_pocket[1], 0.5]);
    }
}

/*///////////////////////
// RENDER SELECTION //
/////////////////////*/
// Uncomment exactly ONE of the following lines to export that part as STL.

// base();   // Export bottom/base
// lid();    // Export lid/top
// frame();  // Export internal frame (optional)
// // To export all three at once, comment/uncomment as needed and run separate exports.

/*///////////////////////
// SNAP FIT TEST (optional) //
/////////////////////*/
// Uncomment to generate a simple test piece: a post and a matching hole.
// Useful for verifying tolerance on your printer.
// test_snap();
// 
// module test_snap() {
//     // Post
//     translate([-20, 0, 0])
//         cylinder(d=led_holder_post_diameter, h=led_holder_post_height);
//     // Hole
//     translate([20, 0, 0])
//         cylinder(d=led_holder_hole_diameter, h=led_holder_post_height);
// }