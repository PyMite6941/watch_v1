# Your watch — owner's guide

This watch was built by hand for you. It is see-through on purpose: everything it does, you can see the part that does it.

## The basics

**It is normally dark.** Press either button and it lights up for a few seconds, then goes back to sleep. That is how a battery this small lasts.

| Button | Press | Hold (about 1 second) |
|---|---|---|
| **MODE** (left) | Next screen | Set the clock by hand |
| **SET** (right) | Depends on the screen — see below | Depends on the screen |
| **Both together, 2 seconds** | | Link mode: turns Bluetooth on so Watch Studio can connect. Shows `LInk`. |

The first press that wakes the watch only wakes it; it does not count as a command.

## Watch Studio, the app on your computer

Your watch came with a small app for Windows or Mac, **Watch Studio**. Install it from the zip with the watch (the `READ ME.txt` inside says how — about a minute, and it takes almost no space). Open it from the Start menu, with **Ctrl + Alt + W**, or on a Mac with Cmd-Space → "watch".

With it you can choose which screens the watch shows, set bells and the time zone, add Wi-Fi so the watch keeps perfect time, make your own little apps for it (a lab step timer, a random name picker, a countdown to exam day), record its sensors to a spreadsheet, and back everything up. Press *Try the demo watch* to explore without the watch.

Connect with a data cable in the **right-hand** USB-C socket, or by Bluetooth (hold both buttons for 2 seconds). Removing the app: Settings → Apps on Windows, or drag it to the Bin on a Mac. The watch keeps all its settings either way.

## Screens

MODE steps through your screens. Which screens you have, and in what order, is chosen on the setup page ([CONFIGURATION.md](CONFIGURATION.md)).

| Screen | Shows | SET press | SET hold |
|---|---|---|---|
| **time** | Hours and minutes. In 12-hour mode the last dot means PM. | Light: next colour | Light: party mode |
| **seconds** | Minutes and seconds | Light | Party mode |
| **date** | Month and day | Light | Sync the time over Wi-Fi now (`SYnC`, then `On` or `Err`) |
| **battery** | `b 87` = 87 %. The dot blinks while charging. | Light | Party mode |
| **stopwatch** | Minutes:seconds, then hours:minutes | Start / stop | Reset to zero |
| **timer** | Time left | Start / cancel | Next length: 1, 2, 3, 5, 10, 15, 20, 25, 30, 45, 60 min |
| **counter** | A tally, 0–9999 | Add one | Reset to zero |
| **dice** | `d  4` | Roll | Roll |
| **period** | Time until the next bell in your schedule | Light | Party mode |
| **mesh** | `P  3` = three other watches of your group in range | Buzz everyone in the group | Same |
| **temp** ★ | `23.4C` | — | Save a sample to the log now (`LOG`) |
| **humidity** ★ | `H 56` = 56 % | — | Save a sample |
| **pressure** ★ | Air pressure in hPa, e.g. `1013` | — | Save a sample |
| **tuner** ✦ | Note, octave, how far off — see below | — | — |
| **metronome** | Beats per minute; it taps your wrist on each beat | Start / stop | Next tempo: 60, 72, 80, 90, 100, 120, 140, 160 |
| **weather** ◆ | `18*C` now; SET steps through the rest — see below | Next: sky, high, low, rain, sunrise, sunset, air | Fetch the weather now |

★ needs the environment pack, ✦ needs the sound pack ([ADDONS.md](ADDONS.md)). Without the pack these screens show `----` or `tunE`.
◆ needs Wi-Fi and a place, set in Watch Studio › Weather.

### Reading the weather

Pressing SET on the weather screen goes round these in turn:

| Shows | Means |
|---|---|
| `18*C` | Temperature now |
| `CLr`, `PArt`, `CLdy`, `FOG`, `drIP`, `rAIn`, `SHOr`, `SnO`, `ICE`, `StOr` | Clear, partly cloudy, cloudy, fog, drizzle, rain, showers, snow, freezing rain, thunderstorm |
| `H 21` / `L 12` | Today's high and low |
| `rAIn`, then `r 20` | Chance of rain today, % |
| `rISE`, then `06:32` | Sunrise |
| `SEt`, then `17:41` | Sunset |
| `AQI`, then `38` | US air quality index: up to 50 is good, over 100 is unhealthy for sensitive groups, over 150 for everyone |

`----` means no weather in the last 12 hours: hold SET to fetch it. `LOC` means no place is set yet. The watch fetches by itself about every hour, but only while the weather screen is one of your screens.

The stopwatch and timer keep running while the watch sleeps. When the timer ends the watch wakes itself, vibrates four times and shows `End`.

The light switches itself off after a minute.

### Reading the tuner

Play one note near the microphone.

```
 A  4  1 2        the note (a dot after the letter = sharp, so "A." is A♯)
 │  │  └─┴─ cents away from perfect pitch (00 to 50)
 │  └─ octave (A4 is the A above middle C, 440 Hz)
 └─ note name: C d E F G A b
```

The light tells you which way to go: **green** = in tune (within 5 cents), **red** = flat, tune up, **blue** = sharp, tune down.

### Bells

If a bell schedule is set, the watch vibrates three times and shows `bELL` at each time, even when asleep. With a warning set, it gives one long buzz that many minutes earlier and shows, for example, `5_` for five minutes left.

## Setting the clock by hand

You normally never need to: the watch sets itself over Wi-Fi. Without Wi-Fi:

1. Hold **MODE** until a pair of digits blinks.
2. **SET** raises the blinking value (hold it to run quickly).
3. **MODE** moves on: hour → minute → month → day → year.
4. After the year it saves and shows `SEt`. Holding MODE saves at once. Doing nothing for 20 seconds cancels.

## Charging

Use the **charging socket on the left-hand side**. The charger board's own light shows through the case: one colour while charging, another when full (usually red, then blue or green). A full charge takes about two hours.

The USB-C socket on the **right-hand side** is for the computer — setup and new firmware. It runs the watch while plugged in but does not charge the battery.

## What the display is telling you

| Display | Meaning |
|---|---|
| `----` on a clock screen, dot blinking | The watch does not know the time. Sync it, or set it by hand. |
| `----` on a sensor screen | No pack plugged in |
| `Lo` on waking | Battery low — charge it |
| `SYnC` | Fetching the time over Wi-Fi (up to about 15 seconds) |
| `Err` after syncing | No saved network in range, or the time server did not answer |
| `LInk` | Bluetooth is on for the setup page |
| `bELL` / `End` | A bell from your schedule / your timer finished |
| Four letters you did not expect | Someone in your group sent you a message |

## Care

- It is **not waterproof**. Take it off for hand-washing at the sink if you splash a lot, and for rain.
- The battery is a lithium cell. Do not crush or puncture the case; if the case ever bulges, stop using and stop charging it.
- If it will sit unused for months, charge it to about half first.
- Stuck? Hold both buttons for link mode and use *Restart watch* on the setup page — or unplug and replug the battery inside.
