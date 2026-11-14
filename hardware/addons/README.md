# Add-on packs: hardware

Plug-in boards for the watch's accessory socket. What the socket is and what it can do is in [docs/ADDONS.md](../../docs/ADDONS.md); this folder holds the boards and the plan for making packs cheaply.

**Status: sketch.** The Pack Base board below is designed, routed and passes KiCad's design-rule check, and its factory files are exported. It has not been ordered, built or plugged into a watch.

| Folder / file | What it is |
|---|---|
| `pack_base/gen_pack_base.py` | Generates the Pack Base board, checks it, and with `--fab` exports the factory files |
| `pack_base/kicad/pack_base.kicad_pcb` | The board, placed and routed. Open it in KiCad 10 |
| `pack_base/fab/pack_base_gerbers.zip` | Upload this to the PCB factory |
| `pack_base/render_top.png`, `render_bottom.png` | Pictures of both sides |

## The idea: one board for every pack

Instead of a different board per pack, every pack is the same **Pack Base** with a cheap ready-made sensor board soldered on top. This keeps costs down three ways:

1. **One design, one order.** PCB factories charge per design. Five Pack Bases come in the same minimum order as one, so there is one order to pay for instead of one per pack.
2. **Order it in the same basket as the watch board,** so they ship together. Shipping usually costs more than the boards themselves.
3. **The sensors are mass-produced breakout boards** that cost little each (see the catalogue). Nothing on the Pack Base itself costs more than a few cents apart from the optional Qwiic socket.

It also makes the gift more versatile, because the Pack Base takes three kinds of add-on:

- **Breakout row (J2):** 6 holes in the order most cheap I²C boards use: `V G SCL SDA` and two spare holes. A GY-BME280 solders straight in, and its two extra pins land in the spare holes. BH1750, GY-521 (MPU6050) and most AHT20 boards start with the same 4 pins. Match the printed labels, not the pin count.
- **Microphone row (J3):** `G`, `X` (AUX) and `A` (AIN), for a MAX4466 or MAX9814 board. Power comes from AUX, so the microphone is off while the watch sleeps.
- **Qwiic / STEMMA QT socket (J5):** the 4-pin plug used by hundreds of SparkFun and Adafruit sensors, and by cheap copies of them. With it, a teacher can buy a new sensor later and plug it in with a cable, with no soldering. Several Qwiic sensors can chain one after another. Leave the socket off to save money; the board works without it.

Every I²C sensor shows up in Watch Studio's *Lab › Scan the socket* the moment it is plugged in, even before the firmware knows how to read it. That makes "what is on the bus?" a lesson on its own.

## The Pack Base board

20 × 18.6 mm, 2 layers, all through-hole except the optional Qwiic socket.

```
   top side, pack plugged in             underneath
   ┌──────── [Qwiic] ────────┐           ┌─────────────────────────┐
   │  V  G  CL DA  -  -      │  J2       │  "AWAY FROM DISPLAY"    │
   │ [3|·|X]            ○ G  │  JP1, J3  │                         │
   │                    ○ X  │           │   2x3 male header (J1)  │
   │      ● 1  ○ 3  ○ 5      │           │   plastic on this side  │
   └──────○ 2──○ 4──○ 6─○ A──┘           └─────────────────────────┘
          edge nearest the display
```

- **J1** is a standard straight 2×3 male header, plastic strip underneath, soldered on top. Its pins are exactly over the watch's socket. The generator checks this against the watch board's coordinates every time it runs.
- **The bottom edge is 1.3 mm below the lower pin row,** so the pack stops short of the display window (it starts 1.8 mm below). The rest of the board sticks out past the top edge of the watch.
- **JP1 sensor power:**
  - The pads come joined between `3` and the centre, so sensors are always powered from 3V3.
  - To power them only while the watch is awake, cut that join with a knife and put a blob of solder between the centre and `X` (AUX).
  - See *Battery life* below before relying on this.
- **Pin 1** has a square pad and a printed `1` on both sides, plus "PIN 1 = LID NOTCH" underneath.

To rebuild after a change:

```bash
"%LOCALAPPDATA%\Programs\KiCad\10.0\bin\python.exe" hardware/addons/pack_base/gen_pack_base.py --fab
```

It prints its placement checks and the design-rule result. It only writes the factory files when both are clean.

### Ordering

Upload `pack_base/fab/pack_base_gerbers.zip`. The defaults are right: 2 layers, 1.6 mm, any colour, HASL (lead-free if offered). Add it to the same basket as the watch board.

The minimum order is usually 5 boards. For more, choose the factory's own panelising (at JLCPCB, "Panel by JLCPCB", 2 × 2) to get 4 boards per panel, which is 20 from the minimum order. Check what it adds to the price; it is usually small.

### Building a pack on it

1. Solder J1 (the 2×3 header) first: plastic underneath, solder on top, trim the pins flush on top.
2. Sensor board: push its own header pins through J2 from the top and solder underneath. Lay the sensor flat over the Pack Base, body towards the display end, with its header's plastic as the spacer. Do not stand it upright; that would make it very tall on a wrist.
3. Microphone board: same at J3, matching `G` / `X` / `A` to the board's GND / VCC / OUT.
4. Qwiic socket, if wanted: solder it last. It is surface-mount with pins 1 mm apart. That is doable with flux and a fine tip, or the PCB factory can fit it.
5. Before the first plug-in, measure resistance between J1 pins 1 and 2: it must not be near 0 Ω.

## Catalogue

"Works now" means the firmware already has a screen for it. "Scan only" means it shows up in Watch Studio's I²C scan, but it needs a small driver before the watch can display its readings. The price bands are rough guesses for cheap marketplace listings, not checked quotes, so compare before buying:

- **$**: under about $2
- **$$**: about $2–6
- **$$$**: $10 and up

| Pack | Module | Firmware | Cost | Good for |
|---|---|---|---|---|
| Environment | GY-BME280 (3.3 V version) | **Works now:** temperature, humidity, pressure, logging | $$ | Biology, environmental science, chemistry; pressure gives altitude, e.g. climbing stairs |
| Environment, no humidity | GY-BMP280 | **Works now:** temperature, pressure | $ | Same, cheaper. Many "BME280" listings actually ship this chip. The watch tells which one it is (no humidity screen) |
| Sound | MAX4466 board (MAX9814 also works) | **Works now:** tuner | $ | Music |
| Humidity, cheaper | AHT20 (often sold on one board with a BMP280) | Scan only (the BMP280 half works now) | $ | Environment pack at the lowest price. **Best next driver:** a few dozen lines |
| Light | BH1750 | Scan only | $ | Photosynthesis, inverse-square law. **Easy driver** |
| Motion | GY-521 (MPU6050); tie AD0 high, address `0x69` | Scan only | $ | Physics: pendulums, acceleration, step counting |
| Distance | VL53L0X | Scan only | $$ | Physics: motion graphs |
| Pulse | MAX30102 | Scan only. Clashes with the EEPROM on some clock modules | $$ | PE, biology: heart rate before and after exercise |
| Air quality | SCD40 (CO₂) | Scan only | $$$ | Chemistry, biology. Only if a teacher really wants it: it costs more than everything else here combined |
| Text display | SSD1306 0.91" OLED | Scan only | $ | Longer messages, labels for readings |
| Beeper | Passive piezo disc between AUX and GND | Needs firmware to drive AUX as a tone | $ | Audible timer alarms, metronome click, tuning reference. Idea only |

Which pack suits which teacher: see [docs/IDEAS.md](../../docs/IDEAS.md).

## Keeping the cost down: recommendations

1. **Give every teacher a Pack Base with the Qwiic socket, plus one "works now" pack for their subject.** The Qwiic socket is what keeps the gift growing after it is handed over.
2. **Make packs a shared library, not one each.** Every watch has the same socket, so packs swap between watches. Buy one of each extra sensor and let teachers borrow, instead of buying every sensor for every watch.
3. **Start with modules the firmware already reads** (BME280/BMP280, MAX4466). Every other module also costs firmware time.
4. **Buy modules in 3- or 5-packs;** the price per board drops a lot. Spares cover any that arrive dead or are fakes.
5. **Next firmware drivers, in order of usefulness per cent:** AHT20 (cheapest humidity), then BH1750 (lux), then MPU6050 (motion).
6. **Perfboard is still free.** A Pack Base is neater and harder to get wrong, but a perfboard pack built to [docs/ADDONS.md](../../docs/ADDONS.md) › *Building a pack so it fits* works too.

## Battery life: watch out for power LEDs

The watch has a 150 mAh battery and sleeps on microamps. Many cheap breakout boards have an always-on power LED drawing 1–3 mA, which on the always-on supply would **flatten the watch in 2–6 days** even when it is not being used. Before fitting any module:

- **Best fix:** remove its power LED, or the LED's resistor (a hot iron and tweezers, or scrape it off with a knife). Then it can stay on 3V3.
- **JP1 set to AUX** turns the module off while the watch sleeps, but not completely. The watch's 4.7 kΩ I²C pull-ups stay powered and can feed a little current into an unpowered module through its data pins. A module left on AUX can still draw up to about 1 mA through its data pins.
- **Check every pack:** with the pack plugged in and the watch asleep (display dark), measure the battery current. A bare watch sleeps in microamps; anything over about 0.1 mA with the pack fitted needs one of the fixes above.

## Not done yet

- [ ] Order and build Pack Bases; check the fit on a printed case (header depth, edge clear of the display window)
- [ ] Measure sleep current with each module, on 3V3 and on AUX
- [ ] Firmware drivers: AHT20, BH1750, MPU6050, then the rest of the catalogue
- [ ] Piezo beeper on AUX: firmware tone output, check that the light sensor reading (which shares AUX) recovers afterwards
- [ ] A printed cap that covers a pack and keys it so it can only go in one way. The pack's direction already gives it away (backwards, it covers the display), but a cap would also protect the sensor
- [ ] Optional: a panel that puts a Pack Base in the corner of every watch-board order, once the watch board's routing is finished
