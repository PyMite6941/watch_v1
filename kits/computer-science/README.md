# Kit: computer science

For a teacher of AP Computer Science A and Principles, cybersecurity, or databases. The watch itself is the lesson material: it is an open, documented computer on a wrist, with a JSON protocol, an I²C bus and a file system to explore.

## What to make

| Part | File / source | Notes |
|---|---|---|
| Watch | [`docs/BUILD_GUIDE.md`](../../docs/BUILD_GUIDE.md) | Standard build |
| Lid | `enclosure/stl/lid_binary.stl` | Bars spelling "CS" in ASCII: long = 1, short = 0. Change `lid_binary_text` for other letters (2–3 fit) |
| Base | Export with `base_text`, such as `hello, world` | Optional |
| Pack | Any. An environment pack (GY-BME280) gives real data for the database lesson | Pack Base: [`hardware/addons/`](../../hardware/addons/README.md) |
| Desk stand | `enclosure/stl/desk_stand.stl` | Timer facing the class |
| Scripts | `watch.py`, `log_to_sqlite.py` in this folder | `pip install pyserial` |

## The preset

- **Screens:** time, seconds, date, counter, timer (5 min), stopwatch, dice.
- **Apps:**
  - `PAIr`: pair programming: swaps between driver 1 and driver 2 every 10 minutes, repeating.
  - `CALL`: a random number from 1 to 30, for cold-calling by seat or roll number.
  - `bIn`: scrolls `01001000 01001001`. Students decode it.

## The scripts

`watch.py` finds the watch on USB and sends it commands:

```
python watch.py                                   # info
python watch.py '{"cmd":"show","text":"HI","ms":3000}'
python watch.py '{"cmd":"i2c_scan"}'
```

It's also a class to import (`from watch import Watch`): about 100 lines, short enough to read in one lesson. It covers:

- serial ports;
- JSON;
- request/response matching by id;
- events arriving mixed in with replies;
- timeouts.

`log_to_sqlite.py` copies the watch's sensor log into `watch_log.db`, in a table `samples(t, temp_c, humidity, hpa, light_mv)`. With `--demo` it makes three days of fake data, so the SQL part works without a watch.

## Ideas for using it

**AP CSP / CSA**

- **The protocol is an API.** All commands are listed in [`docs/CONFIGURATION.md`](../../docs/CONFIGURATION.md) › *The command protocol*. Students write a program that does something new, for example:
  - a quiz that buzzes the watch;
  - a reaction-time game from the `btn` events in `stream`;
  - a scoreboard on the display with `show`.
- **Binary and encodings.** The `bIn` app; the lid pattern; the seven-segment alphabet (why can't it show K or M?); and `i2c_read`, which returns raw bytes that students turn into a temperature using the BME280 datasheet.
- **Abstraction layers.** Follow one button press through all of them:
  - firmware in `firmware/watch_v1/`;
  - the JSON line;
  - Watch Studio (`app/index.html`).

**Cybersecurity**

- **Threat model the watch.** The Bluetooth link is **not encrypted**. The PIN is only a polite lock against a classmate changing settings. Have students work out:
  - what an attacker within radio range could read or change;
  - what a stolen watch gives away (the Wi-Fi passwords are stored on it);
  - what it would take to fix: pairing, encryption, rate-limiting PIN guesses.

  Then compare with a real smartwatch's security documentation.
- **Input validation.** Send malformed commands with `watch.py` and see how the firmware rejects them. Overlong text, wrong types and unknown commands are a safe kind of fuzzing on a device the student owns.
- **Physical access.** The USB port gives full control with no PIN. Ask why that's a reasonable design for this device, and when it wouldn't be.

**Databases**

- **A real data set.** Leave the watch logging in the classroom for a week (`log_min` 10–30), then run `log_to_sqlite.py`. For example:

  ```sql
  SELECT date(t, 'unixepoch') AS day, round(avg(temp_c), 1) AS avg_c, max(temp_c) FROM samples GROUP BY day;
  SELECT strftime('%H', t, 'unixepoch') AS hour, avg(light_mv) FROM samples GROUP BY hour;  -- when are the lights on?
  ```
- **NULLs on purpose.** A BMP280 has no humidity sensor, so that column comes back NULL. This leads into `IS NULL`, `count(*)` versus `count(humidity)`, and `avg` skipping NULLs.
- **Primary keys.** Run the import twice and nothing is duplicated. Ask why (`INSERT OR IGNORE` on `t`).
- **Joining two tables.** Import a second watch into another table, then join on time. For example, inside versus outside.
