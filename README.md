# Watch V1

A see-through, build-it-yourself wrist watch made as a gift for STEM teachers. An ESP32-C3 SuperMini drives a four-digit LED display, an RGB LED and a vibration motor inside a clear printed case. Every watch is set up for its owner with **Watch Studio**, a tiny desktop app for Windows and macOS — no programming — and takes plug-in **packs** for extra abilities (a weather sensor for biology, a microphone tuner for music).

| | |
|---|---|
| Size | 62.8 × 48.8 × 25.3 mm case, 22 mm strap |
| Display | 0.56" four-digit seven-segment, auto-dimming |
| Battery | 150 mAh Li-Po, charged through its own USB socket |
| Radios | Wi-Fi (time sync), Bluetooth LE (setup), ESP-NOW (watch-to-watch) |
| Controls | Two buttons: MODE and SET |
| Expansion | 2×3 accessory socket on the face (I²C, switched power, analogue in) |

## Where things stand

| Part | State |
|---|---|
| Circuit (`hardware/scripts/design.py`) | Done, revision 5. Schematic generated, electrical rules check passes with 0 findings. |
| PCB | All 49 parts placed and checked; an automatic audit confirms every connection against the parts' pinouts. Routing and Gerbers are made in the cloud by a GitHub workflow, so builders never run a router — see [`hardware/README.md`](hardware/README.md). |
| Enclosure | Modelled; STL files exported in `enclosure/stl/`. Not yet test-printed against a real board. |
| Firmware 2.2.0 | Complete and compiles for the ESP32-C3 (custom apps, file space, live data, I²C bridge, weather). **Not yet run on hardware** — expect a debugging session on the first assembled watch. |
| Watch Studio app | Done; tested against its built-in demo watch. Installers for Windows (tested) and macOS (not yet tried on a Mac) in [`installer/`](installer/README.md). Needs a real watch to be tested over USB and Bluetooth. |

The list of what is left is kept outside the repository.

## Start here

| You want to… | Read |
|---|---|
| Use a finished watch | [`docs/USER_GUIDE.md`](docs/USER_GUIDE.md) |
| Install the app | Download from [Releases](https://github.com/PyMite6941/watch_v1/releases) — details in [`installer/README.md`](installer/README.md) |
| Use the app without installing it | Host it on the web with Vercel, or run it as a Java app — [`docs/APP_HOSTING.md`](docs/APP_HOSTING.md) |
| Set one up (Wi-Fi, screens, bells, custom apps) | [`docs/CONFIGURATION.md`](docs/CONFIGURATION.md) |
| Build one | [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md), parts in [`docs/BOM.md`](docs/BOM.md) |
| Order boards and cases | [`docs/ORDERING.md`](docs/ORDERING.md) |
| Add sensors or other hardware | [`docs/ADDONS.md`](docs/ADDONS.md); pack boards and costs in [`hardware/addons/`](hardware/addons/README.md) |
| See what it could do for each subject | [`docs/IDEAS.md`](docs/IDEAS.md) |
| Make one for a particular person (biology, computer science, band, calculus, youth group) | [`kits/`](kits/README.md): preset, pack, engraved case and printed stand for each |
| Change the circuit or board | [`hardware/README.md`](hardware/README.md) |

## What it does

- **Clock**: time, seconds, date, battery level. Keeps time with a DS3231 clock chip and corrects itself over Wi-Fi.
- **Weather** (over Wi-Fi): temperature, sky, high/low, chance of rain, sunrise/sunset and air quality for a place you choose.
- **Classroom tools**: stopwatch, countdown timer, tally counter, dice, bell schedule with a countdown to the next bell, metronome.
- **Packs** (plug into the accessory socket): temperature, humidity and air pressure with a sample log you download as a spreadsheet; an instrument tuner.
- **Watch-to-watch**: watches in the same group can see each other, buzz each other and share the time, with no router.
- **Custom apps**: step timers, random name pickers, countdowns and messages, made in Watch Studio and stored on the watch.
- **Personal**: each owner chooses which screens appear and in what order, brightness, time zone, 12/24-hour, bells, and more.

Battery life comes from being asleep: the watch wakes when a button is pressed, shows a screen for a few seconds and sleeps again. It also wakes on its own for a bell, a finished timer, a log sample or a time sync.

## Repository layout

```
firmware/watch_v1/     Arduino sketch (one file per feature; start with watch_v1.ino)
app/                   Watch Studio, the companion app (one page: index.html)
installer/             Windows and macOS installers for it; build.py makes dist/*.zip
java-app/              the same app as a Java program; build.py makes dist/WatchStudio.jar
vercel.json            serves app/ as a web site on Vercel
hardware/scripts/      design.py is the circuit; the other scripts generate KiCad files from it
hardware/kicad/        generated KiCad 10 project (schematic, board, libraries)
hardware/addons/       add-on packs: the Pack Base board that every pack is built on
enclosure/             OpenSCAD model (case with optional engraving, accessories) and exported STL files
kits/                  ready-made setups for one person each: preset, pack, case pattern, ideas
docs/                  guides
```

## Licence

MIT — see [`LICENSE`](LICENSE). Keep the copyright notice in copies.

Pull requests are welcome when written by a person; automated or bot-generated pull requests are not accepted.
