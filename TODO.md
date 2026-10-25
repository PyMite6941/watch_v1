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
   Refer to the pin definitions in firmware.ino (lines 28-46) and the schematic in src/watch_v1.kicad_sch.

   Summary of connections:
   - Segment pins (a,b,c,d,e,f,g,dp): GPIOs 7,8,9,10,11,12,13,14 → each through a 220 Ω resistor to the corresponding segment on the 5643A-1 display.
   - Digit enable pins (NPN transistor bases): GPIOs 0,2,4,5 → each through a 4.7 kΩ resistor to the base of a 2N2222 transistor; collector to digit common, emitter to GND.
   - Touch buttons: GPIO1 (Mode), GPIO3 (LED) – connect one side of each tactile switch to GPIO, other side to GND (firmware uses internal pull‑up; external pull‑ups optional).
   - LDR: GPIO6 – connect one leg to 3.3 V, the other leg to GPIO6 and also to a 10 kΩ resistor to GND (voltage divider).
   - Rotary encoder (KY-040): CLK → GPIO19, DT → GPIO20, SW → GPIO21 (with pull‑ups, either external 10 kΩ or enable internal in firmware).
   - RGB LED (common cathode): Red anode → GPIO16 via 220 Ω, Green → GPIO17 via 220 Ω, Blue → GPIO18 via 220 Ω; cathode to GND.
   - Vibration motor: Driver NPN (e.g., 2N2222) base → GPIO15 via 1 kΩ, collector → motor+ (other side to VCC via Schottky flyback diode), emitter to GND.
   - Power:
     * USB‑C VBUS → TP4056 IN+.
     * TP4056 OUT+ → Li‑Po BAT+ (through TP4056) and BAT‑ to Li‑Po‑.
     * Li‑Po BAT+ → anode of Schottky diode (1N5819), cathode → MCP1700‑33 VIN.
     * MCP1700‑33 VOUT → ESP32‑C3 VIN and 3.3 V rail.
     * ESP32‑C3 GND → common ground.

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
