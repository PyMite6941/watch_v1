# Watch V1 — Circuit Research & Tutorial Log

Precedent projects and tutorials to copy instead of designing from scratch. Organized by subsystem so you can build each circuit, verify it, then move on.

Source repo: https://github.com/pymite6941/watch_v1
Watch V1 is an ESP32-C3 SuperMini watch: 4-digit 5643A-1 7-segment via one 74HC595, TP4056 LiPo charger → Schottky → 3.3V LDO, LDR auto-brightness, RGB LED, haptics, touch buttons, 3D-printed enclosure, NTP time, deep sleep.

---

## 1. Battery circuit & power switching

How the battery powers everything and how USB takes priority when plugged in.

| # | Project | Link | What it uses | Copy for Watch V1 |
|---|---------|------|--------------|-------------------|
| 1 | Schrödinger Watch (KiCad PCB) | https://github.com/nithinmathewjoji/Schrodinger-Clock-PCB-Watch- | ESP32-S3, KiCad PCB, TP4056, Schottky diode USB protection, P-MOSFET power switching, battery voltage divider ADC | **KiCad power schematic template.** TP4056 + Schottky + LDO + battery-monitor divider on one board. |
| 2 | ESP32-Smartwatch | https://github.com/Cobaltmaster/ESP32-Smartwatch | ESP-WROOM-32, TP4056, AP2210N-3.3 LDO, vibration motor, 400 mAh LiPo | LDO + TP4056 pairing; AP2210N as alternative to MCP1700/AMS1117 |
| 3 | Ella_box | https://github.com/migit/Ella_box | ESP32-C3, TP4056 LiPo charger, 500 mAh LiPo, vibration motor, OLED, rotary encoder | ESP32-C3 + TP4056 + LiPo on one board |
| 4 | NadeshikoSlimeVR | https://github.com/Aeurias/NadeshikoSlimeVR | ESP32-C3 SuperMini, TP4056 unprotected LiPo charger, JST 2-pin battery, slide switch | Snap-fit case + ESP32-C3 SuperMini + TP4056 on one PCB |
| 5 | Smart 18650 Charger w/ OLED | https://hackster.io/Bambu3Design/smart-18650-battery-charger-with-oled-e4940c | ESP32-C3 SuperMini, TP4056 USB-C, INA219, 3D-printed PLA/PETG enclosure | ESP32-C3 SuperMini + TP4056 + 3D-printed enclosure in one build |
| 6 | pgpemu-case | https://github.com/spezifisch/pgpemu-case | TP4056 USB-C charger, 5mm RGB LED + resistors, 10 µF caps, LiPo safety warning (don't charge while on without protection circuit) | LiPo safety note; RGB LED wiring alongside charger |

Watch V1 power chain (from `dist/parts_list.md` / `src/ascii_schematic.txt`):
USB-C VBUS → TP4056 (LiPo charger, PROG ~12 kΩ for ~75–83 mA) → Schottky 1N5819 (USB priority, prevents back-feeding battery→USB) → LDO MCP1700-33 / AMS1117-3.3 (3.3 V rail) → ESP32-C3 + display + peripherals. LiPo 3.7 V 150 mAh JST-PH feeds TP4056 BAT±.

Tutorials to read for this section:
- https://circuitdigest.com/microcontroller-projects/esp32-with-tp4056-li-po-battery-charger (TP4056 usage with ESP32)
- https://learn.adafruit.com/adafruit-mcp4728 (LDO decoupling caps placement — general principle)
- https://lastminuteengineers.com/tp4056-li-ion-battery-charger-tutorial/ (TP4056 PROG resistor math, status LEDs)

---

## 2. Clock counting & 7-segment display

Timekeeping (millis() + NTP), driving the 5643A-1 4-digit 7-segment via 74HC595 shift register + 4 direct digit pins through 2N2222 NPN drivers.

| # | Project | Link | What it uses | Copy for Watch V1 |
|---|---------|------|--------------|-------------------|
| 7 | clock_basic | https://github.com/awalker/clock_basic | ESP8266, 7-segment display, 8-bit shift register | Minimal 7-segment + shift register clock code |
| 8 | wallclock | https://github.com/jcalvinowens/wallclock | ESP32-C3, chained 74HC595 shift registers driving 7-seg LEDs, NTP, 3.3V LDO, Schottky protection | **Closest MCU match.** ESP32-C3 + shift-register 7-segment + NTP |
| 9 | ESP32 + 74HC595 7-segment tutorial | https://circuitdigest.com/microcontroller-projects/interfacing-esp32-with-74hc595-for-7-segment-display | ESP32, 3× 74HC595, 3× 7-seg displays, 3 GPIO control | Wiring pattern: DATA/CLK/LATCH to ESP32; segment resistors |
| 10 | ESP32 + 74HC595 4-digit 7-segment | https://microcontrollerslab.com/esp32-74hc595-4-digit-7-segment-display/ | ESP32, single 74HC595, 4-digit 7-seg, digit pins direct to ESP32 | **Hybrid matches Watch V1 exactly:** 1 shift register for 8 segments + 4 direct digit pins |
| 11 | DIYables 4-digit 7-seg 74HC595 library | https://github.com/DIYables/DIYables_4Digit7Segment_74HC595 | Arduino lib for 4-digit 7-seg + 74HC595, ESP32 tested, HH:MM with blinking dot | Drop-in library to replace hand-rolled mux code |
| 12 | SevSegShift library | https://github.com/bridystone/SevSegShift | Arduino SevSeg + shift register branch; 3 pins for segments, digit pins direct | Alternative library for shift-register 7-seg multiplexing |
| 13 | DIY 7-segment watch (Reddit) | https://www.reddit.com/r/arduino/comments/1an2j3n/build_my_own_seven_segment_display_watch/ | Homemade LED digital watch, buttons to change time | Proof a 7-seg watch is buildable by one person |
| 14 | TM1637 4-digit 7-seg alternative | https://arduinogetstarted.com/tutorials/arduino-tm1637-4-digit-7-segment-display/ | TM1637 driver IC (2-wire) instead of 74HC595 | Simpler 2-pin display option if 74HC595 wiring fails |
| 15 | Seven-segment WiFi Clock ESP32-C3 SuperMini | https://github.com/Abaymelethil/Seven_segment_wifi_clock_esp32 | ESP32-C3 SuperMini, WiFi, NTP, SSD1306 OLED | NTP code for the SuperMini specifically |

Watch V1 firmware (`src/firmware.ino`) already implements: `millis()` second tick, HH:MM / MM:SS / MM:DD modes, blinking colon, 74HC595 segment mux on GPIO 7/8/9, digit commons on GPIO 0/2/4/5 via 2N2222, NTP sync via `esp_sntp.h`, deep sleep after 3 s inactivity.

Tutorials to read for this section:
- https://microcontrollerslab.com/esp32-74hc595-4-digit-7-segment-display/ (full hookup + code)
- https://lastminuteengineers.com/74hc595-shift-register-arduino-tutorial/ (shift register fundamentals)
- https://randomnerdtutorials.com/esp32-ntp-client-date-time-arduino-ide/ (ESP32 NTP time sync)

---

## 3. Lights (RGB LED & haptics)

RGB LED color feedback + party mode, vibration motor haptics via NPN low-side switch with flyback diode.

| # | Project | Link | What it uses | Copy for Watch V1 |
|---|---------|------|--------------|-------------------|
| 16 | Nebula openwatch | https://github.com/Thecactusman0/Nebula-openwatch | Haptic vibration motor for user feedback, ESP32-S3-MINI, LiPo charging | Haptic pulse on button press / mode change |
| 17 | MutantW | https://www.espressif.com/en/news/MutantW | Vibration motor + WS2812B RGB LED + 2 buttons + 3D-printed parts | RGB + haptics feedback combo |
| 18 | ESP32 Smartwatch (CircuitDigest) | https://circuitdigest.com/microcontroller-projects/diy-smartwatch-using-esp32-final-part | 10 mm vibration motor via S8050 NPN transistor, freewheeling/flyback diode across motor | **Haptics circuit:** NPN low-side switch + 1 kΩ base resistor + flyback diode (Watch V1 uses 2N2222 + 1N4148/1N5819) |
| 19 | TTGO T-Watch 2020 (teardown) | https://nick.zoic.org/art/lilygo-ttgo-t-watch-2020/ | GPIO4 PWM to vibration motor at 5–30 Hz for varied buzz patterns | PWM haptics patterns instead of simple on/off |
| 20 | pgpemu-case | https://github.com/spezifisch/pgpemu-case | 5 mm RGB LED + current-limiting resistors | RGB LED resistor values (220 Ω at 3.3 V) |

Watch V1 lights (`src/firmware.ino`): RGB on GPIO 16(R)/17(G)/18(B) via 220 Ω, common cathode; preset colors + HSV party mode; haptics on GPIO 15 via 1 kΩ to 2N2222 base, motor + to 3.3 V, flyback diode cathode to +3.3 V.

---

## 4. Others (inputs, sensing, enclosure, timekeeping, BLE)

| # | Project | Link | What it uses | Copy for Watch V1 |
|---|---------|------|--------------|-------------------|
| 21 | ESP32-C3 & OLED Smartwatch (gist) | https://gist.github.com/HDRobotica/b0418fc0393713ee0247296dacedbc56 | ESP32-C3, OLED, WiFi, NTPClient, deep sleep, 4 touch buttons, Thailand +7h offset | Touch on GPIO1/GPIO3; NTP sync + deep-sleep after inactivity |
| 22 | OpenTimeWatch-1 | https://github.com/OpenTimeWatch-Project/OpenTimeWatch-1 | ESP32-S3, TP4056 charging, 3 nav buttons, LiPo, open-source OS | Open-source smartwatch reference; button layout |
| 23 | NadeshikoSlimeVR case | https://github.com/Aeurias/NadeshikoSlimeVR | 2-part snap-fit case, strap holes, TP4056 + ESP32-C3 USB port cutouts | Snap-fit two-part enclosure; port cutout placement |
| 24 | OG DIY SmartWatch | https://elecrow.com/sharepj/og-diy-smartwatch-1327.html | ESP32-C3, SLA/DLP resin case, M1.6 screws, 20 mm quick-release straps, JST battery pocket | Resin case + strap mounts + battery pocket |
| 25 | WeatherPaper enclosure | https://hackster.io/jeremyngai/weatherpaper-101fa6 | TinkerCAD, snap-on lid, JLC3DP SLA frosted resin, translucent finish | Translucent/resin front panel so display shows through |
| 26 | ESP32 Smartwatch (CircuitDigest) — ambient light | https://circuitdigest.com/microcontroller-projects/diy-smartwatch-using-esp32-final-part | BH1750 ambient light sensor, auto-brightness PWM backlight, hole in enclosure for sensor | Auto-brightness logic; sensor placed behind front panel hole |
| 27 | esp32-cyberclock | https://github.com/somebox/esp32-cyberclock | I2C ambient light meter, automatic brightness adjustment | Ambient-light → brightness feedback loop |
| 28 | Schrödinger Watch — RTC | https://github.com/nithinmathewjoji/Schrodinger-Clock-PCB-Watch- | DS3231 RTC with backup battery, alarm | Add DS3231 RTC if you want time kept without WiFi/NTP |

Watch V1 "others" already implemented:
- Touch buttons: GPIO1 (MODE), GPIO3 (LED), INPUT_PULLUP
- LDR: GPIO6 ADC + 10 kΩ pull-down, brightnessFactor 0.2–1.0 scaling digit pulse width
- Encoder (optional): GPIO19 CLK, GPIO20 DT, GPIO21 SW
- BLE: `BLEDevice::init("WatchV1")` placeholder
- Enclosure: `assets/enclosure.scad` OpenSCAD base/lid/frame, 60×40×20 mm, display window, battery pocket, LED holder, USB-C cutout

---

## Build order (KiCad-free → optional PCB)

1. **Breadboard power:** TP4056 → Schottky → LDO → 3.3 V rail + 150 mAh LiPo (sections 1)
2. **Breadboard display:** ESP32-C3 + 5643A-1 + one 74HC595 + 8× 220 Ω + 4× 2N2222 + 4× 4.7 kΩ (section 2)
3. **Add lights/inputs:** RGB GPIO16/17/18, haptics GPIO15+2N2222+1k+diode, touch GPIO1/3, LDR GPIO6 (sections 3 & 4)
4. **Enclosure:** print `assets/enclosure.scad` base/lid; clear/resin front panel (section 4)
5. **Optional PCB later:** copy KiCad power schematic from #1 only for charger/LDO; keep rest on perfboard

Files already in this repo to reuse:
- `src/firmware.ino` — complete firmware
- `src/ascii_schematic.txt` + `dist/NETLIST.csv` — wiring/netlist
- `dist/parts_list.md` — full BOM with Thai supplier links
- `assets/enclosure.scad` + `assets/PLAN.md` — OpenSCAD enclosure
