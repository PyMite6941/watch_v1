# Add-ons

Hardware is added through the **accessory socket** on the watch face. A plug-in board for it is called a *pack*.

The **Pack Base**, one small board that every pack can be built on, and the plan for making packs cheaply are in [hardware/addons/](../hardware/addons/README.md). The perfboard instructions below still work if you would rather not order boards.

The two USB sockets are not for add-ons: the one on the left only charges, and the USB-C on the right is for a computer (setup and firmware).

## The accessory socket

A 2×3 female header (J4, 2.54 mm pitch) reached through an opening in the lid, top right of the display. A pack is a small board with a 2×3 male header that pushes in.

Looking at the watch face, strap ends up and down:

```
            top edge of the watch
        ┌───────┬───────┬─────────┐
        │ 1 GND │ 3 SDA │ 5 AUX   │
        ├───────┼───────┼─────────┤
        │ 2 3V3 │ 4 SCL │ 6 AIN   │
        └───────┴───────┴─────────┘
                 display below
```

| Pin | Name | What it is | Limits |
|---|---|---|---|
| 1 | GND | Ground | |
| 2 | 3V3 | 3.3 V, **always on** — also while the watch sleeps | Keep sleeping current in microamps or it drains the battery. Up to ~100 mA while awake. |
| 3, 4 | SDA, SCL | I²C bus, 3.3 V, 100 kHz, shared with the clock chip | Address `0x68` is taken (DS3231), and `0x57` if your clock module carries an EEPROM |
| 5 | AUX | 3.3 V that is **on only while the watch is awake** (driven straight from a processor pin) | **20 mA at most.** Use it to power things that should cost nothing in sleep. |
| 6 | AIN | Analogue input for audio-rate signals | AC-coupled through 100 nF: it sees changes, not steady voltages. Keep it to about 1 V peak-to-peak (see below). |

How AIN works, because it is unusual: the processor has no spare analogue pin, so capacitor C4 lays the AIN signal on top of the battery-measurement pin. The firmware averages that pin to read the battery and looks at the wiggle to hear sound. A steady voltage on AIN therefore reads as nothing; for slow sensors use I²C.

What that means for a signal on AIN:

- The pin sits at half the battery voltage, 1.6–2.1 V, and the processor reads up to about 2.5 V. A signal can swing only about 0.4 V upward on a full battery before its tops are cut off, so **about 1 V peak-to-peak is the most that is useful**. The tuner copes with some clipping, but a smaller signal reads better.
- C4 and the 50 kΩ battery divider form a high-pass filter at about 32 Hz. Anything slower fades out.
- While a loud signal is playing, the battery reading wobbles a little. The firmware averages 32 readings, which hides most of it.

AUX comes from GPIO21, which is also the processor's start-up message pin. For a few milliseconds at every wake it flickers before it settles high. A microphone does not mind; a pack that latches or counts on power-up should wait about 50 ms.

Polarity matters and the socket itself is not keyed, so pin 1 is marked three ways: a **round notch in the lid opening** at the pin 1 corner (top-left), a printed `1` beside the socket on the board, and a square pad on the back. Mark pin 1 on every pack you build — a dot of paint on that corner — and line the dot up with the notch.

### Building a pack so it fits

The socket sits right at the top edge of the watch, just above the display. These measurements come from the board and case files.

```
   pack board seen from above, plugged in
   ┌───────────────────────────┐
   │                           │   ← the board sticks out past the top
   │      sensor / mic board   │     edge of the watch, over the strap
   │                           │
   ├ ─ ─ ─ watch's top edge ─ ─ ┤   3.8 mm from here to the top row of pins
   │       ● 1   ○ 3   ○ 5     │
   │       ○ 2   ○ 4   ○ 6     │
   └───────────────────────────┘   at most 1.5 mm below the lower row,
   ═════ display window ═════════   or it covers the top of the digits
```

- **Which way the board goes.** The pack's board points away from the display, out over the top edge of the watch. The display window begins about 1.8 mm below the lower row of pins. On perfboard, put the header in the bottom two rows and file the edge back to about half a hole below them. The board can be up to about 20 mm wide, centred on the socket.
- **Which side to solder.** Push the header's plastic strip against the *underside* of the pack and solder on the top. Seen from the top with the pack pointing away from the display, the pins then sit exactly as in the socket diagram above: pin 1 top-left, next to the notch. If you wire on the underside instead, everything is mirrored left to right. That is the most common way to build a pack backwards.
- **Header.** Use an ordinary straight 2.54 mm male header (2.5 mm plastic, 6 mm pins below it), not a right-angle one. The socket's top is 3.6 mm below the outside of the lid. The plastic strip passes through the 8.6 × 6.2 mm opening, the pack board rests on the lid, and the pins go about 5 mm into the socket. That holds firmly without glue.
- **When to plug in.** Plug in or unplug while the watch is asleep (display dark). Pin 2 is always powered, and a pack's capacitors charging up can restart an awake watch. The watch looks for sensors every time it wakes, so the new pack is found the next time you press a button.
- **If it goes in backwards** (rotated half a turn, so pin 1 lands on pin 6): the environment pack simply isn't found, and nothing is harmed. The sound pack's output ends up connected straight to ground. Unplug it at once; a few seconds is normally survivable.

Check a new pack before its first plug-in. Put a multimeter on resistance between the pack's pins 1 and 2, then 1 and 5: neither should read near 0 Ω. A reading near 0 Ω means a solder bridge would short the watch's power supply.

### Environment pack — temperature, humidity, pressure

For the biology/environmental-science watch: live readings and a log of samples.

| Part | Notes |
|---|---|
| BME280 breakout, **3.3 V I²C version** | Often sold as "GY-BME280". A BMP280 looks identical and also works, but measures no humidity. |
| 2×3 male pin header | |
| Scrap of perfboard, about 15 × 20 mm | |

| BME280 pin | Socket pin |
|---|---|
| VIN / VCC | 2 (3V3) |
| GND | 1 (GND) |
| SCL | 4 (SCL) |
| SDA | 3 (SDA) |

Leave CSB and SDO unconnected (address `0x76`; `0x77` is also recognised). The sensor idles at well under a microamp, so permanent 3V3 is fine.

Turn on the `temp`, `humidity` and `pressure` screens. For logging set *Log a sample every … minutes*; the watch wakes by itself, records temperature, humidity, pressure and light level, and sleeps. It holds the latest 240 samples (10 days at one an hour) and Watch Studio downloads them as a CSV file for a spreadsheet. Holding SET on a sensor screen records a sample on the spot.

Accuracy notes for lessons: the sensor sits a few millimetres above a wrist and an LED display, so worn readings run warm. For real measurements take the watch off, let it settle for a few minutes, or put the sensor on a short ribbon cable. The log lives in memory that survives sleep but not a flat or disconnected battery — download before the battery runs out.

### Sound pack — instrument tuner

| Part | Notes |
|---|---|
| MAX4466 electret microphone amplifier board | Has a gain trimmer on the back. A MAX9814 board also works. |
| 2×3 male pin header, perfboard scrap | |

| Microphone board pin | Socket pin |
|---|---|
| VCC | 5 (AUX) — so it draws nothing while the watch sleeps |
| GND | 1 (GND) |
| OUT | 6 (AIN) |

Turn on the `tuner` screen. Start with the gain trimmer about a third of the way up, because AIN can only take about 1 V peak-to-peak (see *The accessory socket*); if the tuner ignores quiet notes turn the gain up, if it jumps about turn it down. The tuner covers about 60–1000 Hz (voice, guitar, violin, most winds; a bass guitar's lowest string and a piccolo's top notes are out of range). `tune_a4` moves the reference pitch for ensembles that tune to 442 and the like.

This is the newest and least-proven feature: the pitch maths is standard, but it has not been tried on real hardware yet. It is on the project to-do list.

### Designing your own pack

Anything that speaks I²C at 3.3 V can share the socket. Parts that would suit this watch, none of which have firmware yet:

| Part | Address | Would give |
|---|---|---|
| MAX30102 | `0x57` (clashes with the EEPROM on some clock modules) | Pulse and blood oxygen — heart rate before/after exercise |
| MPU6050 / LIS3DH | `0x68`\* / `0x18` | Acceleration: pendulum timing, step counting, tilt-to-wake |
| VEML7700 / BH1750 | `0x10` / `0x23` | Light in lux for photosynthesis and inverse-square experiments |
| SCD40 | `0x62` | CO₂ — classroom air quality, respiration |
| SSD1306 OLED 0.49" | `0x3C` | A text display for longer messages |
| VL53L0X | `0x29` | Distance by laser — motion graphs |

\* The MPU6050's default address collides with the clock chip; tie its AD0 pin high to move it to `0x69`.

To add one to the firmware: detect it in `setup()` the way `envBegin()` does, add a screen name to `View` in `types.h` and `VIEW_NAMES` in `settings.h`, and draw it in `renderView()` in `watch_v1.ino`. `env.h` is the model to copy.

## Why not the USB-C port?

The ESP32-C3's USB is a fixed-function **device**: to a computer it is a serial port, and that is all it can be. It cannot act as a USB **host**, so a USB gadget plugged into the watch — microphone, flash drive, keyboard, MIDI instrument — does nothing, and no firmware can change that.

What the port is good for is a computer or phone talking to the watch with the commands in [CONFIGURATION.md](CONFIGURATION.md). Watch Studio is the first example; a lesson-specific page (a live graph of the sensor, a class timer dashboard) would work the same way.

Hardware add-ons through the USB-C port are a possible future project and are on the project to-do list; nothing in the current design depends on them.

## What goes where

| You want to attach | Use |
|---|---|
| A sensor, a small display | Accessory socket (I²C) |
| A microphone or other audio-rate signal | Accessory socket (AIN) |
| A laptop or phone for setup or a classroom page | USB-C on the right, or Bluetooth |
| A charger | The charging socket on the left, only |
