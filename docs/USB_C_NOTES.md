# USB‑C Considerations for ESP32‑C3 SuperMini Watch

## Power Path Overview
The watch is designed to be powered and programmed via the USB‑C connector on the ESP32‑C3 SuperMini module. The ASCII schematic and Build Guide show the following power flow:

```
USB‑C VBUS (5 V) → TP4056 IN → Li‑Po BAT+/BAT‑ → Schottky diode → 3.3 V LDO → ESP32‑C3 VIN & 3.3 V rail
```

- The ESP32‑C3 SuperMini provides a USB‑C receptacle that supplies 5 V VBUS when connected to a host or charger.
- The TP4056 Li‑Po charger module is used to charge the battery from that 5 V source.
- A Schottky diode prevents back‑feeding from the battery to the USB‑C port and gives USB priority when plugged in.
- The 3.3 V LDO (MCP1700‑33 or similar) regulates the voltage for the ESP32‑C3 and display.

## TP4056 Module Compatibility
The parts list describes the TP4056 as a **micro‑USB** Li‑Po charger, but the schematic and Build Guide treat its input as USB‑C VBUS. In practice you have two options:

1. **Use a TP4056 module with a USB‑C input** (many suppliers sell “TP4056 USB‑C” boards).  
   - If you obtain such a board, connect its USB‑C receptacle directly to the ESP32‑C3’s USB‑C VBUS (you can wire VBUS to the module’s 5 V pin, or simply plug a USB‑C‑to‑USB‑C cable between the ESP32‑C3 and the charger if the charger has a USB‑C receptacle).

2. **Use a standard micro‑USB TP4056** and add a USB‑C‑to‑micro‑USB adapter or a short cable.  
   - Solder the adapter’s micro‑USB end to the TP4056 IN pins (or plug the adapter into the module).  
   - Connect the adapter’s USB‑C plug to the ESP32‑C3’s USB‑C port (or to a separate USB‑C wall wart that then feeds the adapter).  
   - Ensure the adapter can supply at least 500 mA (the TP4056 PROG resistor sets charge current; see parts list for calculation).

> **Note:** The Build Guide step 16 assumes the TP4056 IN pin is directly tied to USB‑C VBUS. If you use an adapter, keep the wiring short and robust.

## Enclosure Cut‑out for USB‑C
The ESP32‑C3 SuperMini’s USB‑C connector must be accessible for programming and charging. When designing the PCB or placing the module on perfboard:

- Position the module so its USB‑C port faces outward (as mentioned in Build Guide step 2).
- In the 3D‑printed enclosure (see `assets/enclosure.scad`), either:
  - Leave a hole in the top or side wall large enough for the USB‑C plug (≈ 8.5 mm × 2.5 mm), or
  - Design a sliding cover or snap‑fit flap that exposes the port when needed.
- The provided OpenSCAD file does not currently include a USB‑C cut‑out; you can add one by modifying the `outer_length`, `outer_width`, or adding a custom module (see the “LED holder” parameters as an example).

## Recommended Work‑flow
1. **Confirm your TP4056 variant** (USB‑C or micro‑USB).  
2. If micro‑USB, obtain a reliable USB‑C‑to‑micro‑USB adapter or cable.  
3. Update the schematic in KiCad (`watch_v1.kicad_sch`) to reflect the exact connector you will use (add a USB‑C connector symbol if needed, or note the adapter).  
4. Assign footprints:  
   - ESP32‑C3 SuperMini: use the pre‑defined footprint (or create one matching the module’s dimensions).  
   - TP4056: use the appropriate footprint (micro‑USB or USB‑C).  
   - Add a USB‑C connector footprint if you integrate one directly on the PCB.  
5. Run ERC and DRC, route the board, keeping the USB‑C traces wide enough for 500 mA–1 A (0.5 mm width is sufficient for short runs).  
6. Generate Gerbers and order a 2‑layer PCB (60 mm × 40 mm as per Build Guide).  
7. After PCB receipt, solder components, ensuring the USB‑C connector is not obstructed by the enclosure.  
8. Flash firmware via the USB‑C port; verify charging and operation as per the Build Guide testing steps.

## Open Items
- [ ] Verify TP4056 module USB‑C pinout (if using USB‑C version) and add to schematic.  
- [ ] Add USB‑C cut‑out to `assets/enclosure.scad` or document manual modification.  
- [ ] Update `parts_list.md` to disambiguate TP4056 variant (e.g., “TP4056 USB‑C Li‑Po charger module” or note adapter requirement).  
- [ ] If integrating a USB‑C connector directly on the PCB, add its footprint and coupling to ESP32‑C3 VBUS.

By clarifying the USB‑C power path and ensuring mechanical access, the watch can be successfully powered, programmed, and charged via a single USB‑C cable.