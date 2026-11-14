# What each watch could do — ideas by subject

A menu for deciding how to set up each teacher's watch, and where the project could go next.

- ✅ **Ready** — in firmware 2.0.0; just turn the screen on in Watch Studio.
- 🔌 **Needs a pack** — in the firmware, plus a plug-in board ([ADDONS.md](ADDONS.md)).
- 💡 **Idea** — not built. Each notes what it would take.

What the hardware gives you to work with: four digits, one RGB light, a vibration motor, two buttons, a light sensor, accurate time, Wi-Fi, Bluetooth, watch-to-watch radio, and whatever plugs into the accessory socket.

## Every teacher

| | Feature | Use |
|---|---|---|
| ✅ | Bell schedule + "next bell" countdown | A buzz on the wrist at each period change; a glance shows minutes left without looking at the wall clock mid-sentence |
| ✅ | Warning before the bell | "Five minutes left" as a silent nudge to start wrapping up |
| ✅ | Countdown timer | Timed tasks, group-work rounds, test sections — ends with a buzz, no phone alarm |
| ✅ | Stopwatch | Presentations, races, how long the class really took to settle |
| ✅ | Dice with any number of sides | Set the sides to the class size: a fair random student picker |
| ✅ | Tally counter | Participation counts, hands raised, behaviour points |
| ✅ | Group buzz | Two teachers on duty or co-teaching can silently signal each other |
| 💡 | Pomodoro / interval timer | Work–break cycles that repeat. *Firmware only: a second timer length and a loop.* |
| 💡 | Do-not-disturb light | A steady red or green as "ask me / not now" during tests. *Firmware only: a light that stays on in sleep costs battery — would need a blink.* |
| 💡 | Countdown to a date | Days to exams or holidays. *Firmware only.* |
| 💡 | Phone notifications | A buzz for calls or calendar events. *Needs a phone app to push them over Bluetooth; the watch would also have to stay connected, which costs battery.* |

## Computer science

| | Feature | Use |
|---|---|---|
| ✅ | The command protocol | Students send JSON over USB from Python or a web page and the watch obeys: a real, safe device to script. `{"cmd":"show","text":"HI"}` is a first program with a visible result. |
| ✅ | Open source, small firmware | Each feature is one readable file; reading `tuner.h` or `mesh.h` is a lesson in itself |
| ✅ | Watch-to-watch radio | Demonstrate broadcast, addressing and "who can hear whom" with real packets |
| ✅ | Seven-segment display | Bit patterns you can see: `glyph()` in the firmware is a lookup table from characters to bits |
| 💡 | Binary / hexadecimal clock | Time shown as hex digits, or as bits on the segments. *Firmware only; a small screen.* |
| 💡 | Unix-time screen | The last four digits of the epoch ticking. *Firmware only.* |
| 💡 | Reaction-time game | Light turns green, press: milliseconds on the display. Sorting a class's results is a statistics exercise. *Firmware only.* |
| 💡 | Student-written screens | A tiny scripting hook so a class can add its own screen without reflashing. *Substantial firmware work.* |
| 💡 | Quiz buzzer | First watch to press wins, decided over the mesh. *Firmware: needs a timestamped message and a tie-break.* |

## Biology and environmental science

| | Feature | Use |
|---|---|---|
| 🔌 | Temperature, humidity, air pressure | Microclimates around the school: sun and shade, classroom and greenhouse, transpiration in a sealed bag |
| 🔌 | Sample log to spreadsheet | Leave the watch somewhere for a day, download a CSV, graph it. Pressure against the weather; temperature and humidity moving in opposite directions through a day. |
| ✅ | Light level in the log | The light sensor is recorded with every sample: day length, cloud, shade under a canopy |
| ✅ | Tally counter | Quadrat counts, cells in a field of view, birds at a feeder |
| ✅ | Timer | Staining steps, incubation, reaction times |
| ✅ | Metronome | Pacing step tests and breathing-rate experiments |
| 💡 | Pulse | Heart rate before and after exercise. *Needs a MAX30102 pack and a driver.* |
| 💡 | Tap-to-count pulse | Tap SET on each beat felt at the wrist for 15 s; the watch shows beats per minute. *Firmware only — no sensor.* |
| 💡 | CO₂ | Classroom air through a lesson; respiration in a sealed container. *Needs an SCD40 pack and a driver.* |
| 💡 | Altitude | Pressure to height for field trips. *Firmware only, with the environment pack.* |
| 💡 | Field-sample marker | Hold SET to stamp a numbered sample with time and readings, so samples match the notebook. *Mostly there: holding SET already logs; needs a sample number in the log.* |

## Mathematics and calculus

| | Feature | Use |
|---|---|---|
| ✅ | Dice with any sides | Probability experiments: roll a d6 a hundred times as a class and compare with theory |
| ✅ | Tally counter and stopwatch | Collecting real data to model |
| 🔌 | Logged temperature or pressure | Real, slightly noisy curves for rates of change, numerical derivatives, area under a curve |
| 💡 | Random numbers with a chosen distribution | Uniform is there; normal or binomial would let a class see a histogram form. *Firmware only.* |
| 💡 | Digits of π or e | Scroll through them four at a time. *Firmware only; a party piece.* |
| 💡 | Cooling-curve mode | Log temperature every few seconds for ten minutes for Newton's law of cooling. *Firmware: logging is per-minute today; needs a fast mode.* |
| 💡 | Pi-day / e-day countdown | *Firmware only.* |

## Physics and chemistry

| | Feature | Use |
|---|---|---|
| ✅ | Stopwatch | Pendulums, trolleys, reaction rates |
| 🔌 | Air pressure | Pressure against height on a staircase; a syringe and Boyle's law |
| 🔌 | Tuner as a frequency meter | Tuning forks, resonance tubes, strings under different tension — the pitch is shown as a note; the frequency itself is available in the firmware |
| 💡 | Frequency in hertz on screen | *Firmware only: a second tuner layout.* |
| 💡 | Sound level | Decibel-ish meter for the inverse-square law or classroom noise. *Firmware only, with the sound pack; needs calibration to mean anything.* |
| 💡 | Light meter in lux | Inverse-square law with a lamp. *Needs a VEML7700 pack; the built-in sensor is not linear enough.* |
| 💡 | Acceleration | Free fall, circular motion, pendulum period by counting swings. *Needs an accelerometer pack and a driver.* |
| 💡 | Distance | Motion graphs by walking towards a wall. *Needs a VL53L0X pack and a driver.* |

## Music

| | Feature | Use |
|---|---|---|
| 🔌 | Tuner | Note, octave and cents with a red / green / blue light for flat / in tune / sharp |
| ✅ | Metronome on the wrist | A silent pulse the player feels and the audience does not hear |
| ✅ | Adjustable reference pitch | A = 440, 442, baroque 415 — anything from 400 to 480 Hz |
| 💡 | Tap tempo | Tap SET in time and the metronome takes that tempo. *Firmware only.* |
| 💡 | Time signatures | A stronger buzz on beat one. *Firmware only.* |
| 💡 | Reference tone | Play an A. *Needs a small piezo on the accessory socket — AUX can drive one directly.* |
| 💡 | Rehearsal sync | One watch sets the tempo for the section over the mesh. *Firmware: a tempo message.* |

## Suggested starting set-ups

| Teacher | Screens, in order | Extras |
|---|---|---|
| Computer science | time, period, timer, dice, mesh, counter | Mesh on; show them the Serial Monitor trick on day one |
| Biology | time, period, temp, humidity, pressure, counter, timer | Environment pack; log every 15 min |
| Calculus / maths | time, period, timer, stopwatch, dice | Dice sides = class size |
| Physics / chemistry | time, period, stopwatch, timer, pressure, tuner | Both packs |
| Music | time, tuner, metronome, timer, period | Sound pack |

Save each as a file with Watch Studio's *Backup* so a set-up can be restored or shared.

## Choosing what to build next

Roughly in order of value for effort — all firmware-only, no new parts:

1. Tap tempo and accented first beat (music)
2. Tap-to-count pulse (biology)
3. Reaction-time game (computer science, maths)
4. Frequency in hertz on the tuner (physics)
5. Fast logging mode (maths, physics)
6. Binary / hex clock (computer science)

These are tracked on the project to-do list.
