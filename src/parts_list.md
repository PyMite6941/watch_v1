# Parts List for ESP32-C3 SuperMini Watch (Watch V1)

## Core Modules
- ESP32-C3 SuperMini development board (USB‑C, 3.3 V) – Main microcontroller; runs firmware, multiplexes display, reads buttons/LDR, handles Bluetooth.
- 5643A‑1 4‑digit 7‑segment LED display with colon (common cathode) – Shows time/date; driven via segment resistors and NPN digit drivers.
- TP4056 micro‑USB Li‑Po charger module (adjustable charge current) – Charges the Li‑Po battery from USB‑C; charge current set by PROG resistor.
- Schottky diode (e.g., 1N5819) – USB power priority – Prevents back‑feeding from battery to USB and gives USB priority when plugged in.
- 3.3 V LDO regulator (e.g., MCP1700‑33‑T0/TO‑92 or AMS1117‑3.3) – Provides a stable 3.3 V supply to the ESP32‑C3 and display from battery/USB.
- Touch buttons: use ESP32-C3 built‑in touch sensing on GPIO1 (MODE) and GPIO3 (LED) (no external parts). Mechanical buttons optional.
- Jumper wires / breadboard for prototyping – Used to assemble and test the circuit before soldering or PCB fabrication.
- Perfboard or custom PCB (optional) – Final platform for mounting components; perfboard for quick build, PCB for a clean, replicable design.

## Power Storage
- Li‑Po battery, 3.7 V, 150 mAh, JST‑PH 2‑pin connector – Selected for best balance of capacity and size.
  - **Recommended product:** YDL 3.7V 150 mAh LiPo Battery 551521 (or equivalent 150 mAh JST‑PH cell).
    - Dimensions ≈ 23 × 15 × 5.5 mm, comfortably fits inside the 60 × 40 × 20 mm enclosure alongside the PCB.
    - Charge current target: ~0.5 C → 75 mA. Set via TP4056 PROG resistor: \\\\(R_{PROG} = 1000 / I_{CHG}\\\\) → 1000/75 ≈ 13.3 kΩ; use nearest standard 12 kΩ (≈83 mA) or 15 kΩ (≈66 mA). Either is safe; 12 kΩ gives slightly faster charge.
  - If 150 mAh is unavailable, a 120 mAh cell (e.g., YDL 3.7V 120 mAh 302030) works as a fallback; adjust PROG resistor accordingly (~10 kΩ for 100 mA, ~15 kΩ for ~66 mA).
- Photoresistor (LDR) – e.g., GL5528 or similar (≈10 kΩ dark, ≈1 kΩ bright) – Senses ambient light for auto‑brightness; firmware adjusts display pulse width based on reading.

## Passive Components
- Resistors:
- 8 × 220 Ω (segment current limit) – 1/4 W or 1/8 W – Limits current through each LED segment to ~5‑6 mA at 3.3 V.
- 4 × 4.7 kΩ (NPN transistor base bias) – 1/4 W – Biases the bases of the four NPN transistors that drive the digit commons.
- 1 × 10 kΩ (LDR pull‑down) – 1/4 W – Pull‑down resistor for the LDR on GPIO6.
- 1 × 12 kΩ (TP4056 PROG) – sets charge current ≈ 83 mA for the 150 mAh Li‑Po (safe 0.55 C).
- Capacitors (optional for LDO stability):
  - 1 × 1 µF ceramic (LDO input) – Decouples the LDO input.
  - 1 × 1 µF ceramic (LDO output) – Decouples the LDO output.
  - 1 × 0.1µF ceramic (shift register decoupling) – Decouples the 74HC595 VCC‑GND.


## Active Components
- NPN transistors (e.g., 2N2222) ×4 – Used as low‑side switches for the digit commons (multiplexing).
- 74HC595N 8‑bit shift register (e.g., SN74HC595N) – Drives the 7‑segment display segments via serial‑in, parallel‑out, reducing GPIO usage to three pins (DATA, CLK, LATCH).  
  *Example Lazada/Shopee:* 1 pcs for ฿69 (Shopee, iseerobot.th) or 50 pcs for ฿465 (Lazada, rthgu654dfj).

## Enclosure
- Clear PETG or acrylic sheet (front “glass” wall) – cut to ~60 mm × 40 mm – Transparent front panel so the internal circuitry is visible.
- Clear PETG or resin for side walls (can be 3D printed) – Forms the remaining walls of the see‑through case.
- 3D‑printed battery pocket (PETG or PLA) – dimensions ~30 mm × 15 mm × 5 mm – Holds the Li‑Po battery securely inside the enclosure.
- Filament needed: ~3.5–4 m of 1.75 mm PETG (≈10 g) for enclosure (base, lid, optional frame).
- O‑ring or silicone gasket (optional for water resistance) – Provides a basic seal against moisture/splashes.
- Small screws or snap‑fit design for assembly – Mechanically joins the front, sides, and back of the enclosure.

## Tools & Consumables
- Soldering iron & thin solder (if using perfboard/PCB) – For making permanent connections.
- Wire strippers, tweezers – Assist with preparing and placing wires and components.
- Heat‑shrink tubing or electrical tape – Insulates solder joints and wires.
- Isopropyl alcohol & conformal coating spray (optional waterproofing) – Cleans the PCB and adds a protective moisture‑resistant layer.
- Double‑sided foam tape or adhesive for securing display – Holds the 5643A‑1 module in place without obstructing light.


## Optional Components
- KY-040 rotary encoder (optional) – Connect CLK to GPIO19, DT to GPIO20, SW to GPIO21 (with pull‑ups). Firmware ignores it by default; enable in code if needed.
## Wireless Charging Add‑on (optional)
- Wireless charging receiver module (e.g., RXS5V-1000 or similar) – provides 5 V output from Qi coil.
- Connect VOUT to TP4056 IN+ (USB‑C VBUS) and GND to TP4056 IN‑ (or directly to battery via Schottky diode if preferred).
- Ensure the coil fits inside enclosure; may require thin ferrite sheet.
- No firmware changes needed; TP4056 handles charging.

## Notes
- All components are through‑hole or breadboard‑friendly; no fine‑pitch SMD required except the ESP32‑C3 module (pre‑mounted).
- For water resistance, apply a thin conformal coating to the assembled PCB and seal seams with silicone rubber.
- The clear enclosure lets the internal circuitry be visible; ensure the display is visible through the front panel.
- Charge current setting:
  - TP4056 PROG resistor: \\\\( I_{CHG} = \\\\\\\\frac{1000}{R_{PROG}} \\\\\\\\) (mA).
  - Selected 12 kΩ gives ~83 mA (safe for 150 mAh Li‑Po).
- Battery connector: JST‑PH 2‑pin (1.0 mm pitch) avoids soldering directly to the cell.
- **Using the Elegoo R3 Uno Arduino kit for prototyping**:
  - You can temporarily replace the ESP32‑C3 with an Arduino Uno (5 V) to test the display multiplexing and button logic.
  - Use level‑shifters or resistor dividers for the 5 V Uno to the 3.3 V display if needed, or run the display at 5 V with appropriate resistor values (re‑calculate segment resistors for 5 V Vcc).
  - Once the firmware logic is verified, migrate to the ESP32‑C3 for low‑power operation and Bluetooth capability.
- **Ambient‑light auto‑brightness**:
  - Connect the LDR as shown in the ASCII schematic: one side to 3.3 V, the other side to GPIO6 (ADC) with a 10 kΩ pull‑down to GND.
  - In firmware, read the ADC value, map it to a brightness factor (0.2‑1.0) that scales the digit‑enable pulse width.
  - This provides dimmer display in dark environments and brighter in light, saving power and improving readability.

## RGB LED & Diffuser
- Common cathode RGB LED (e.g., 5mm diffused) – Provides color feedback and party mode.
- 3 × 220 Ω resistors (current limit for R, G, B channels) – 1/4 W – Connect each anode to ESP32 GPIO via resistor; cathode to GND.
- LED dome/diffuser (optional) – A small hemispherical dome that fits over the RGB LED to spread the light evenly, especially useful in party mode.
## Haptics
- Vibration motor (coin type, ~10mm diameter, 3V rated) – Provides haptic feedback for button presses, alarms, or notifications.
- Diode (e.g., 1N4148 or Schottky 1N5819) – Flyback diode across vibration motor to suppress back EMF.
- NPN transistor (e.g., 2N2222) – Low‑side switch to drive the vibration motor from an ESP32 GPIO.
- Resistor, 1 kΩ – Base current limit for the transistor.

## Touch Buttons
- Use the ESP32-C3 built‑in touch sensor on GPIO1 (MODE) and GPIO3 (LED). No extra parts required.
- Firmware uses touchRead with a threshold (see firmware).
- Mechanical buttons can be used alternatively: connect one side to GPIO, other side to GND, and enable internal pull‑up in firmware (comment out touch lines and restore pinMode).
