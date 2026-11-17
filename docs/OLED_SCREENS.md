# OLED screens: designing what the small screen shows

With a Screen Pack (or any SSD1306 OLED) on the accessory socket, the watch shows each of its screens on the OLED too, in words and pictures. Every OLED screen is a **design**: a small text file that says where the clock, the icons, the bars and the words go. The watch has built-in designs for everything. Teachers can make their own in **Watch Studio › Screens**, without writing any code.

- Hardware: [ADDONS.md › Screen pack](ADDONS.md#screen-pack--a-small-oled-that-says-it-in-words) and [hardware/addons/screen_pack/](../hardware/addons/README.md#the-screen-pack-board).
- Status: the drawing code is tested on a computer. The C++ the watch runs is checked against Watch Studio's preview, pixel for pixel, on every change (`.github/workflows/oled-parity.yml`). It has **not yet run on a real watch with a real panel**.

## Making one in Watch Studio

1. Connect, open **Screens**, press **New screen**.
2. Pick a starting point: a blank screen, one of the watch's built-in designs, or a ready-made one (big clock, class bell, name badge, weather, step timer, shout-out).
3. Click a part to pick it and drag it to move it. Arrow keys nudge it by one pixel. The panel on the right changes the text, font, size and so on.
4. **+ Picture…** turns any image (a school logo, a mascot) into OLED dots. Photos look best with dots (dithering); logos and line drawings with plain black and white.
5. **Use it for** chooses which watch screen it replaces: the time, the next bell, the weather, every app, one app, and so on.
6. **When** can limit a design to certain times and days: a school-day face from 07:30 to 15:30 Monday to Friday, and a different one for evenings and weekends.
7. **Try on the watch for 15 s** shows it on the real OLED without saving. **Save to the watch** keeps it.

### Themed clock faces

**New screen** offers ready-made clock faces, grouped:

- **Animals:** bee (its wings flap), fox, cat, dog, owl and rabbit (they blink every few seconds).
- **Subjects:** maths graph, science atom, music notes, open book, seedling.
- **Number fun:** a maths clock that writes the time as sums ("3x4 : 6^2+6", new sums every minute), a binary and hex clock, and a Roman-numeral clock.

Each is an ordinary design: pick one, change anything, save. The pictures travel inside the saved file, so they cost the watch's firmware nothing. **+ Ready-made picture** adds any of the pictures to any design. There are versions for 128×32 and 128×64 panels; 64×32 panels have none yet.

**Show the watch's OLED now** asks the watch to draw its current OLED picture and send it back. It works even with no panel plugged in, so a design can be checked before the hardware arrives.

To go back to a built-in design, remove the file in **Screens**. Designs are ordinary files in `/oled/` on the watch, so **Backup** includes them, and a design downloaded from one Studio can be opened in another.

## Which design the watch uses

For the screen on show, the watch looks for the first of these:

1. a file in `/oled/` whose `for` is that screen (`time`, `weather`, `app:quiz`, `set`, `overlay`…);
2. for an app, a file whose `for` is `app` (every app);
3. the built-in design for that screen, or the built-in "card" (icon, value, label).

In step 1, a file with `when` that applies right now wins over one without `when`. Outside its times it is skipped. A file with `when` is never used until the watch knows the time.

Only files made for the panel's size are used (`w` and `h` must match the setting `oled`). Files whose names start with `_` are skipped: Studio's "try" file is `/oled/_try.json`.

## The file format

```json
{"v": 1, "name": "Class bell", "for": "period", "w": 128, "h": 32, "inv": false,
 "items": [
   {"k": "icon", "n": "bell", "z": 16, "x": 0, "y": 8},
   {"k": "text", "s": "{bell}", "f": [5, 4, 3], "a": "c", "x": 74, "y": 26, "mw": 104, "if": "bell"},
   {"k": "text", "s": "No more bells", "f": [3, 2, 0], "a": "c", "x": 74, "y": 21, "if": "!bell"}
 ]}
```

| Key | Meaning |
|---|---|
| `v` | Format version, 1 |
| `name` | Shown in Studio |
| `for` | Which screen it replaces: a screen name from the `views` setting, `app` (every app), `app:<file name without .json>` (one app), `set` (setting the time) or `overlay` (the short flash after a button press) |
| `w`, `h` | Panel size in pixels: 128×32, 128×64 or 64×32 |
| `inv` | `true` swaps lit and dark over the whole screen |
| `when` | Optional: `{"from": "07:30", "to": "15:30", "days": "12345"}`. Days are digits, Monday = 1 … Sunday = 7; leave `days` out for every day. `from` later than `to` runs over midnight; equal means all day |
| `items` | The parts, drawn in order: later ones go on top |

Coordinates are pixels from the top-left corner. Numbers are whole numbers between −4096 and 4096. Anything after the decimal point is dropped.

### Parts

Every part has `k` (its kind), `x` and `y`. Two optional keys work on any part:

- `c`: colour, `1` lit (the default) or `0` dark. Dark parts cut out of lit ones, such as words on a filled box.
- `if`: show the part only when a field has a value: `"if": "bell"`. With `!` in front, only when it is blank: `"if": "!bell"`. A value of `0` counts as blank.

| `k` | Keys | Draws |
|---|---|---|
| `text` | `s` text with `{fields}`; `f` font number, or a list tried in turn until the text fits; `a` `l`/`c`/`r` (left of, centred on, right of `x`); `y` is the **baseline** (bottom of the letters); `mw` widest in pixels (default: panel width); `wrap` up to this many lines (max 8); `lh` line spacing; `u` capitals | Words. Text that does not fit even in the last font ends in `..` |
| `icon` | `n` icon name, or a field such as `{wx_icon}`; `z` 8, 16 or 32 (32 is the 16 px icon doubled) | One of the built-in icons |
| `battery` | `w` (20), `h` (9) | A battery outline filled to `{batt}` |
| `bar` | `v` a percentage, usually a field such as `{sec_pct}`; `w`, `h`, `r` corner radius; `o` outline (on bars 5 px or taller, default 1) | A progress bar |
| `box` | `w`, `h`, `r` corner radius, `fill` | A rectangle |
| `line` | `x2`, `y2` | A straight line |
| `circle` | `r` radius, `fill` | A circle centred on `x`, `y` |
| `bmp` | `w`, `h` (each at most 256), `d` base64 bitmap: rows top to bottom, 8 pixels a byte, left pixel in the top bit | A picture |

### Fonts

| `f` | Font | Height of a capital, px |
|---|---|---|
| 0 | Pixel 5×7, the classic small font | 7 |
| 1 | Tiny caps (Org_01): reaches 1 px below the baseline | 5 |
| 2 | Sans 9 | 13 |
| 3 | Sans bold 9 | 13 |
| 4 | Sans bold 12 | 18 |
| 5 | Sans bold 18 | 26 |
| 6 | Sans bold 24 | 34 |

They come from the Adafruit GFX library. Studio carries a copy in `app/oled-assets.js`, so its preview uses exactly the same letter shapes. Printable ASCII and `°` are drawn. Any other character shows as `?`.

### Fields

`{name}` in text, in an icon name or in a bar's value is replaced by the watch's live value. A name the watch does not know stays as typed, so a typo shows on the screen. Several fields are blank when there is nothing to show; that is what `if` is for.

| Field | Value |
|---|---|
| `time`, `hh`, `mm`, `ss`, `ampm` | The time (`--:--` when not set), its parts, AM/PM (blank on a 24-hour clock) |
| `day`, `dy`, `dd`, `mon`, `month`, `year`, `date` | Monday, Mon, 16, Nov, November, 2026, Mon 16 Nov |
| `sec_pct` | How far through the current minute, % |
| `batt`, `charging` | Battery %, 1 while charging |
| `wx_temp`, `wx`, `wx_icon`, `hi`, `lo`, `rain` | Weather: 31°C, Partly cloudy, icon name, today's high and low, chance of rain %. Blank without fresh weather |
| `temp`, `hum`, `hpa`, `unit` | Environment Pack readings, and C or F |
| `bell` | Time to the next bell; blank after the last one |
| `timer`, `timer_pct` | Countdown timer, and how far it has run, % |
| `sw`, `tally`, `dice`, `bpm` | Stopwatch, tally counter, last dice roll, metronome |
| `note`, `cents` | Tuner: A4, +3 |
| `name` | The watch's name (Settings) |
| `title`, `text`, `step`, `step_pct` | In an app: its title, a message app's text, a step timer's "2/5" and how far through the current step, % |
| `value`, `label`, `icon` | What the built-in card shows for the current screen: the main number or word, what it is in words, and a fitting icon |
| `view` | The current screen's name |
| `greet` | Good morning / Good afternoon / Good evening |
| `tick`, `blink` | `1` every other second; `1` for a moment every 7 s. Use them with `if` to swap two pictures, for flapping wings or blinking eyes |
| `hh_math`, `mm_math` | The hour and minutes as a sum: `3x4`, `9+3`, `2^2+8`, `24/2`, `15-3`. A new sum every minute |
| `hh_bin`, `mm_bin` | In binary: 4 digits for a 12-hour clock's hour, 5 for 24-hour, 6 for minutes |
| `hh_hex`, `mm_hex` | In hexadecimal, two digits |
| `hh_rom`, `mm_rom` | In Roman numerals; `N` (nulla) for zero |

### Icons

Built in, 16 px: clock, calendar, hourglass, stopwatch, tally, dice, bell, people, thermo, drop, gauge, note, metronome, sun, moon, cloud, partly, rain, snow, storm, fog, chat, list, star, heart, flask, book, pencil, apple, check, wifi, bolt, bee, code, pi, cross, trophy, alarm.

8 px: bolt, bell, sun, moon, cloud, partly, rain, snow, storm, fog, wifi, people, note, drop, thermo, heart, star, check, clock, hourglass, calendar.

## For developers

| File | What it is |
|---|---|
| `tools/oled/icons.txt` | The icons as text art (`#` lit, `.` dark). Edit freely |
| `tools/oled/builtin.json` | The watch's built-in designs |
| `tools/oled/starters.json` | The ready-made classroom designs in Studio |
| `tools/oled/faces.json` | The themed clock faces: one layout per panel size, applied to every picture |
| `tools/oled/draw_art.py` | Draws the faces' pictures from simple shapes at 32 and 48 px and writes `art.txt`. Hand-drawn pictures go in `art_hand.txt` |
| `tools/oled/build_assets.py` | Turns those, plus the GFX fonts, into `firmware/watch_v1/oled_assets.h` and `app/oled-assets.js`. Run it after changing any of them |
| `firmware/watch_v1/oled_render.h` | The watch's renderer |
| `app/oled.js` | Studio's renderer: a line-by-line twin of `oled_render.h` |
| `tools/oled/parity/` | Renders about 360 designs with both and compares them. CI runs it on every change |

The two renderers draw every shape with their own pixel loop instead of the GFX library's shape functions, so the results are identical. A change to one must be made to the other. The font list is fixed by number: add new fonts at the end only.

Link commands (also in [CONFIGURATION.md](CONFIGURATION.md#the-command-protocol)):

- `{"cmd":"oled_show","path":"/oled/x.json","ms":15000}` shows a design for a while, whatever is on screen (1 s to 2 min).
- `{"cmd":"oled_shot"}` returns `w`, `h`, `panel` (is one plugged in), `design` (`f:<path>` or `b:<n>`, built-in) and `data`: the current picture, base64, in the `bmp` format above.
