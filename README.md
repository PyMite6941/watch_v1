# Watch V1

A see-through, build-it-yourself wrist watch made as a gift for STEM teachers. An ESP32-C3 SuperMini drives a four-digit LED display, an RGB LED and a vibration motor inside a clear printed case. Every watch is set up for its owner from a web page — no programming — and takes plug-in **packs** for extra abilities (a weather sensor for biology, a microphone tuner for music).

| | |
|---|---|
| Size | 62.8 × 48.8 × 25.3 mm case, 22 mm strap |
| Display | 0.56" four-digit seven-segment, auto-dimming |
| Battery | 150 mAh Li-Po, charged through its own USB socket |
| Radios | Wi-Fi (time sync), Bluetooth LE (setup), ESP-NOW (watch-to-watch) |
| Controls | Two buttons: MODE and SET |
| Expansion | 2×3 accessory socket on the face (I²C, switched power, analogue in); USB-C for setup and firmware |

## Where things stand

| Part | State |
|---|---|
| Circuit (`hardware/scripts/design.py`) | Done, revision 5. Schematic generated, electrical rules check passes with 0 findings. |
| PCB | All 47 parts placed and checked. **Not routed yet** — see [`hardware/README.md`](hardware/README.md). No Gerbers until that is done. |
| Enclosure | Modelled; STL files exported in `enclosure/stl/`. Not yet test-printed against a real board. |
| Firmware 2.0.0 | Complete and compiles for the ESP32-C3. **Not yet run on hardware** — expect a debugging session on the first assembled watch. |
| Configurator | Written; needs a real watch to be tested end to end. |

[`TODO.md`](TODO.md) is the live list of what is left.

## Start here

| You want to… | Read |
|---|---|
| Use a finished watch | [`docs/USER_GUIDE.md`](docs/USER_GUIDE.md) |
| Set one up (Wi-Fi, screens, bells) | [`docs/CONFIGURATION.md`](docs/CONFIGURATION.md) |
| Build one | [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md), parts in [`docs/BOM.md`](docs/BOM.md) |
| Order boards and cases | [`docs/ORDERING.md`](docs/ORDERING.md) |
| Add sensors or other hardware | [`docs/ADDONS.md`](docs/ADDONS.md) |
| See what it could do for each subject | [`docs/IDEAS.md`](docs/IDEAS.md) |
| Change the circuit or board | [`hardware/README.md`](hardware/README.md) |

## What it does

- **Clock**: time, seconds, date, battery level. Keeps time with a DS3231 clock chip and corrects itself over Wi-Fi.
- **Classroom tools**: stopwatch, countdown timer, tally counter, dice, bell schedule with a countdown to the next bell, metronome.
- **Packs** (plug into the accessory socket): temperature, humidity and air pressure with a sample log you download as a spreadsheet; an instrument tuner.
- **Watch-to-watch**: watches in the same group can see each other, buzz each other and share the time, with no router.
- **Personal**: each owner chooses which screens appear and in what order, brightness, time zone, 12/24-hour, bells, and more.

Battery life comes from being asleep: the watch wakes when a button is pressed, shows a screen for a few seconds and sleeps again. It also wakes on its own for a bell, a finished timer, a log sample or a time sync.

## Repository layout

```
firmware/watch_v1/     Arduino sketch (one file per feature; start with watch_v1.ino)
tools/configurator/    index.html - the setup page (open in Chrome or Edge)
hardware/scripts/      design.py is the circuit; the other scripts generate KiCad files from it
hardware/kicad/        generated KiCad 10 project (schematic, board, libraries)
enclosure/             OpenSCAD model and exported STL files
docs/                  guides
```

## Licence

MIT — see [`LICENSE`](LICENSE). Keep the copyright notice in copies.

Pull requests are welcome when written by a person; automated or bot-generated pull requests are not accepted.
