# Add-ons

The watch has two places to attach things:

1. the **accessory socket** on the face — the intended way to add hardware;
2. the **USB-C port** on the right-hand side — for computers, and with limits explained [below](#the-usb-c-port).

Charging has its own socket on the left and is not involved in either.

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

Polarity matters and the socket is not keyed. Mark pin 1 on every pack and on the lid.

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

Turn on the `temp`, `humidity` and `pressure` screens. For logging set *Log a sample every … minutes*; the watch wakes by itself, records temperature, humidity, pressure and light level, and sleeps. It holds the latest 240 samples (10 days at one an hour) and the configurator downloads them as a CSV file for a spreadsheet. Holding SET on a sensor screen records a sample on the spot.

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

This is the newest and least-proven feature: the pitch maths is standard, but it has not been tried on real hardware yet. See [`TODO.md`](../TODO.md).

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

## The USB-C port

### What it can and cannot be

The ESP32-C3's USB is a fixed-function **device**: it always appears to a computer as a serial port plus a debug probe. It cannot act as a USB **host**, so it cannot power up and talk to USB gadgets — a USB microphone, flash drive, keyboard or MIDI instrument plugged into the watch will do nothing. No firmware can change that; it is how the chip is built.

So "an add-on on the USB-C port" has to be one of these:

| Option | How it works | Effort | Verdict |
|---|---|---|---|
| **A. A computer or phone is the add-on** | The other end is the USB host and speaks the JSON commands in [CONFIGURATION.md](CONFIGURATION.md). The configurator is the first example; a lesson-specific page (live graph of the sensor, class timer dashboard, quiz buzzer) is the same idea. | Low — works with today's firmware | **Do this first** |
| **B. A dock with its own brain** | A small board with a host-capable microcontroller (ESP32-S3, RP2040) plugs in, powers the watch and exchanges the same commands — it can carry a big display, SD card, speakers, anything. | Medium: a second firmware | Good for a desk stand / classroom base station |
| **C. Re-use the port's two data wires as plain signals** | With USB switched off in firmware, the D+ and D− contacts become two ordinary processor pins (GPIO19 and GPIO18). A passive add-on on a USB-C plug could then be a second I²C bus or a serial link, powered from the port. | High, and untested | Possible; see below |

### Option C in detail (not built)

Why it is tempting: sensors could hang off a standard cable with no lid opening. What it would take:

- **Power.** The port's power pin is fed from the battery through a diode, so an add-on gets about 3.4–3.9 V, not 5 V. Use parts that accept that range, or add a small regulator on the add-on.
- **Firmware.** Disable the USB controller's hold on the pads, then start I²C or UART on GPIO18/19. The watch must decide which mode to be in — the natural rule is "if a computer answers within a second or two, stay USB; otherwise try add-on mode".
- **Risks.** While in add-on mode the watch is invisible to a computer; recovery is to hold the SuperMini's BOOT button while plugging in, which always brings USB back for flashing. Plugging an add-on-mode watch into a computer is harmless but will not connect until it switches back. A USB-C plug has many contacts for a hand-soldered add-on; a "USB-C male breakout board" makes it practical.
- **Status.** Reasoned from the chip's documentation, never tried. It is listed in [`TODO.md`](../TODO.md) as an experiment for after the first watch works.

The accessory socket gives the same capabilities (I²C plus power) today with none of those risks, which is why it exists.

## Summary: what goes where

| You want to attach | Use |
|---|---|
| A sensor, a small display | Accessory socket (I²C) |
| A microphone or other audio-rate signal | Accessory socket (AIN) |
| A laptop, a phone, a classroom dashboard | USB-C, option A — or Bluetooth |
| A stand with a screen or speakers | USB-C, option B |
| A charger | The charging socket on the left, only |
