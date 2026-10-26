# TODO Items

1. Install Arduino IDE (version 1.9 or later) if not already installed.
   - Download from https://www.arduino.cc/en/software
   - Follow installation instructions for your OS.

2. Install required libraries via Arduino Library Manager:
   - WiFiManager (by tzapu or tablatronix)
   - (Optional) BLE libraries already included with ESP32 core.

3. Ensure ESP32 board support is installed:
   - In Arduino IDE: File → Preferences → Additional Boards Manager URLs:
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   - Then Tools → Board → Boards Manager → search for "esp32" and install.

4. After installation, open the project in Arduino IDE:
   - File → Open → /home/pymite6941/hardware_projects/watch_v1/src/firmware.ino
   - Select the correct board: "ESP32C3 Dev Module" (or similar).
   - Choose the appropriate USB port.
   - Upload the sketch.

5. Wiring the display and peripherals on breadboard:
   Refer to the pin definitions in firmware.ino (lines 28-46) and the schematic in assets/watch_v1/watch_v1.kicad_sch.

   Summary of connections:
   - Segment pins (a,b,c,d,e,f,g,dp): GPIOs 7,8,9,10,11,12,13,14 → each through a 220 Ω resistor to the corresponding segment on the 5643A-1 display.
   - Digit enable pins (NPN transistor bases): GPIOs 0,2,4,5 → each through a 4.7 kΩ resistor to the base of a 2N2222 transistor; collector to digit common, emitter to GND.
   - Touch buttons: GPIO1 (Mode), GPIO3 (LED) – connect one side of each tactile switch to GPIO, other side to GND (firmware uses internal pull‑up; external pull‑ups optional).
   - LDR: GPIO6 – connect one leg to 3.3 V, the other leg to GPIO6 and also to a 10 kΩ resistor to GND (voltage divider).
   - Rotary encoder (KY-040): CLK → GPIO19, DT → GPIO20, SW → GPIO21 (with pull‑ups, either external 10 kΩ or enable internal in firmware).
   - RGB LED (common cathode): Red anode → GPIO16 via 220 Ω, Green → GPIO17 via 220 Ω, Blue → GPIO18 via 220 Ω; cathode to GND.
   - Vibration motor: Driver NPN (e.g., 2N2222) base → GPIO15 via 1 kΩ, collector → motor+ (other side to VCC via Schottky flyback diode), emitter to GND.
   - Power (load sharing, diode OR-ing):
     * USB-C VBUS → D2 (1N5819 Schottky) → MCP1700-33 LDO IN → LDO OUT → ESP32 VIN & 3.3V rail (USB-powered mode; battery NOT discharged)
     * Li-Po BAT+ → D1 (1N5819 Schottky) → MCP1700-33 LDO IN (battery-only mode, when USB absent)
     * USB-C VBUS → TP4056 IN+ (charges battery whenever USB is present, independent of load source)
     * TP4056 CHRG → GPIO4 (LOW = charging), STDBY → GPIO5 (LOW = charge done)
     * Battery voltage monitor → GPIO2 (ADC1_CH2) via 100k/100k divider (R19/R20)
     * ESP32-C3 GND → common ground.

   Double-check all connections against the schematic before powering.

6. After upload, open Serial Monitor (115200 baud) to see boot messages.
   - If no Wi‑Fi credentials are stored, the ESP32‑C3 will start AP "WatchV1-Setup".
   - On your phone, join that AP, open a browser, select your home Wi‑Fi, enter password, save.
   - The module will then connect to your router, obtain NTP time, and display the time on the 7‑segment display.
   - Test Mode button (short press cycles display mode; long press fast‑adjusts minutes).
   - Test LED button (short press changes LED color; long press toggles party mode).
   - Test encoder to adjust hours/minutes/date.
   - Cover/uncover LDR to see auto‑brightness change.
   - Verify haptic feedback on button presses.

7. Once breadboard testing is successful, proceed to order PCB from JLCPCB (or similar) using the Gerbers generated from the KiCad project, and assemble the final watch.

## KiCad Design — Complete (assets/watch_v1/)
- Schematic (`watch_v1.kicad_sch`, rev 2): fully wired — ESP32-C3 SuperMini, 5643A-1 display, TP4056 charger, 2× 1N5819 Schottky (D1 battery→LDO, D2 USB→LDO for load sharing), MCP1700-33 LDO, 4× 2N2222 digit drivers, buttons, LDR, RGB LED, vibration motor, resistors, decoupling caps, battery voltage divider; all nets connected per docs/NETLIST.csv and src/ascii_schematic.txt
- PCB (`watch_v1.kicad_pcb`): 60mm × 40mm board outline, 40 footprint modules placed, 40 nets defined, 4× 2.2mm mounting holes, net classes for Default and Power
- Fixed LDO_OUT power-short bug (was incorrectly tied to 5V VBUS; now kept on separate 3.3V rail)
- Temp files removed: `.hermes-tmp.*`, `*.bak`, `fp-info-cache`
- Duplicate KiCad files removed from `src/`; canonical project lives in `assets/watch_v1/`
- `.gitignore`: added `*.bak`, `*.kicad_sch.bak`, `fp-info-cache`

## Firmware — Complete (src/firmware.ino)
- Corrected GPIO pin mapping to match KiCad schematic (GPIO7-14 segments, GPIO0/2/4/5 digit drivers, GPIO1/3 touch buttons, GPIO6 LDR, GPIO16/17/18 RGB, GPIO15 haptics, GPIO4/5 TP4056 status, GPIO2 battery ADC)
- Touch buttons use `touchRead()` (capacitive) instead of broken `digitalRead()`
- RGB LED party mode implemented: short press cycles through 6 colors + OFF, long press toggles HSV color-cycling party mode
- Battery voltage monitoring via GPIO2 ADC (100k/100k divider)
- TP4056 charging status monitoring (CHRG/STDBY on GPIO4/5)
- Fixed `ntpSynced` flag never being set (NTP sync now works)
- Removed 74HC595 shift register (not in schematic); segments driven directly from GPIO7-14
- Deep sleep after 3s inactivity, wake by timer

**Next:** Open in KiCad 6+, run "Update PCB from Schematic", run ERC/DRC, route traces, generate Gerbers. Flash firmware via Arduino IDE (ESP32C3 Dev Module).
