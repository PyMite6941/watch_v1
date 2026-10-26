# USB‑C Considerations for ESP32‑C3 SuperMini Watch

## Power Path Overview (with load sharing)

The watch uses diode OR‑ing for load sharing between USB and battery:

```
USB‑C VBUS (5V) ──[D2: 1N5819]──┐
                                ├──→ MCP1700‑33 LDO IN → 3.3V → ESP32‑C3
Li‑Po BAT+ (3.7–4.2V) ──[D1: 1N5819]──┘
TP4056 IN+ ←── USB‑C VBUS (charges battery whenever USB is present)
```

**How it works:**
- **USB connected:** D2 conducts (~4.7V after Schottky drop), D1 is reverse‑biased (battery ~3.4–3.9V). The load runs from USB. The TP4056 charges the battery simultaneously. Battery is **not** discharged.
- **USB disconnected:** D2 is off, D1 conducts. The battery seamlessly powers the load through the LDO.
- **Charge status:** TP4056 CHRG (GPIO4, LOW = charging) and STDBY (GPIO5, LOW = charge done) are monitored by firmware.

**Voltage considerations:**
- MCP1700‑33 dropout: needs ~3.5V+ input for 3.3V output. Battery below ~3.8V may cause LDO dropout — watch will shut down, which is expected end‑of‑life behavior.
- Battery voltage is monitored via GPIO2 (ADC1_CH2) through a 100k/100k divider (R19/R20). 4.2V Li‑Po reads ≈2.1V at the ADC.

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