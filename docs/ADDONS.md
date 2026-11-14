# Add-ons

Hardware is added through the **accessory socket** on the watch face. A plug-in board for it is called a *pack*.

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
| 6 | AIN | Analogue input for audio-rate signals | AC-coupled through 100 nF: it sees changes, not steady voltages. Keep the signal within 0–3.3 V. |

How AIN works, because it is unusual: the processor has no spare analogue pin, so capacitor C4 lays the AIN signal on top of the battery-measurement pin. The firmware averages that pin to read the battery and looks at the wiggle to hear sound. A steady voltage on AIN therefore reads as nothing; for slow sensors use I²C.

Polarity matters and the socket itself is not keyed, so pin 1 is marked three ways: a **round notch in the lid opening** at the pin 1 corner (top-left), a printed `1` beside the socket on the board, and a square pad on the back. Mark pin 1 on every pack you build — a dot of paint on that corner — and line the dot up with the notch.

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

Turn on the `tuner` screen. Start with the gain trimmer in the middle; if the tuner ignores quiet notes turn the gain up, if it jumps about turn it down. The tuner covers about 60–1000 Hz (voice, guitar, violin, most winds; a bass guitar's lowest string and a piccolo's top notes are out of range). `tune_a4` moves the reference pitch for ensembles that tune to 442 and the like.

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
