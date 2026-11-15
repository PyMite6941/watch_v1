# Setting up a watch

Every watch keeps its own settings in its flash memory. You change them with **Watch Studio**, the companion app; nothing needs to be compiled.

## Watch Studio

Watch Studio is a small desktop app for Windows and macOS (about 0.1 MB installed). It opens in its own window from the Start menu, Spotlight, Launchpad, the Dock or a keyboard shortcut. How it is installed, customised and removed: [`installer/README.md`](../installer/README.md). The short version for the person receiving the watch is in the `READ ME.txt` inside each installer zip.

1. Install it: unzip `WatchStudio-Windows.zip` and double-click `Setup.cmd`, or unzip `WatchStudio-macOS.zip` and right-click `Setup Watch Studio.command` → Open. (No installer to hand? Opening [`app/index.html`](../app/index.html) in Chrome or Edge gives the same app in a browser tab.)
2. Connect one of three ways:
   - **USB** — plug a *data* cable into the USB-C socket on the watch's right-hand side, press a watch button to wake it, click *Connect by USB* and pick the port (it is listed as "USB JTAG/serial debug unit"). While it is plugged into a computer the watch stays awake.
   - **Bluetooth** — hold both watch buttons for 2 seconds until it shows `LInk`, click *Connect by Bluetooth* and pick `Watch-<name>`. Link mode ends 3 minutes after the last command.
   - **Demo** — a pretend watch inside the app, to try everything without one.
3. The first time a computer meets a watch, **Quick setup** asks for four things — name, greeting, time zone (guessed from the computer), a Wi-Fi network — and sets the clock from the computer. Everything else is on the tabs:

| Tab | What it is for |
|---|---|
| Watch | Battery, time, sync, Wi-Fi networks, test buzz, show text, Quick setup again |
| Settings | Screens and their order, name and greeting, time zone, brightness, timers, bells, time source, mesh group, PIN |
| Apps | Make extra screens without programming: step timers, random pickers, countdowns, scrolling messages (`/apps/*.json`, see `firmware/watch_v1/apps.h`) |
| Files | The watch's own small file space (about 190 kB): upload, edit, download, delete |
| Lab | Live sensor readings, record to CSV, download the sample log, I²C scan of whatever is in the accessory socket |
| Backup | Save settings and files to one `.json` file; restore it onto this or another watch. Wi-Fi passwords and the PIN are not included |
| Console | Type raw commands (below) and see the traffic |

To give several watches the same set-up: set one up, *Backup*, then *Restore* that file onto each of the others.

### Wi-Fi

The watch only uses Wi-Fi to fetch the time, for a few seconds at a go. Save up to five networks (home, school, a phone hotspot); it scans and joins the strongest one it knows. It needs ordinary password networks — it cannot click through a login web page, and it does not support username-and-password (enterprise) school networks. A phone hotspot is the usual way round that.

Only 2.4 GHz networks are visible to it.

### The PIN

Anyone near a watch in link mode could connect by Bluetooth. Set a PIN and Bluetooth clients must unlock first. USB never asks for it: holding the cable means holding the watch. The PIN is a courtesy lock, not strong security — the link itself is not encrypted.

## Settings reference

| Key | Meaning | Range / default |
|---|---|---|
| `name` | Watch name: Bluetooth name, and what other watches see | 16 characters / `Watch` |
| `hello` | Greeting shown for a moment when the watch wakes from being switched off (seven-segment letters) | 4 characters / empty |
| `tz` | Time zone as a POSIX string. Quick setup fills it in from the computer's clock; new watches start on `UTC0` | `EST5EDT,M3.2.0,M11.1.0` (US Eastern), `<+07>-7` (UTC+7) |
| `h24` | 24-hour clock | `true` |
| `awake_s` | Seconds the display stays on after a press | 3–60 / 5 |
| `auto_bright` | Brightness follows the light sensor | `true` |
| `bright_min`, `bright_max` | Dimmest and brightest level; `bright_max` is the fixed level when `auto_bright` is off | 1–255 / 20, 255 |
| `haptic` | Vibrate on button presses | `true` |
| `views` | Screens, in order | list of `time seconds date battery stopwatch timer counter dice period mesh temp humidity pressure tuner metronome` |
| `timer_s` | Countdown length, seconds | 5–5999 / 300 |
| `dice` | Dice sides | 2–99 / 6 |
| `bpm` | Metronome tempo | 30–240 / 100 |
| `tune_a4` | Tuner reference pitch, Hz | 400–480 / 440 |
| `fahrenheit` | Temperature in °F | `false` |
| `log_min` | Save a sensor sample every N minutes (0 = off) | 0–1440 / 0 |
| `alerts` | Bell times, `"HH:MM"`, at most 16 | empty |
| `alert_days` | Days the bells ring: add 1 Sun, 2 Mon, 4 Tue, 8 Wed, 16 Thu, 32 Fri, 64 Sat | 62 (Mon–Fri) |
| `alert_warn_min` | Extra warning buzz this many minutes before each bell | 0–30 / 0 |
| `time_src` | `ntp` or `http` — see below | `ntp` |
| `ntp` | NTP server | `pool.ntp.org` |
| `time_url`, `time_key` | For `http`: address, and the JSON field holding Unix time | empty |
| `resync_h` | Hours between automatic time syncs (0 = never) | 0–2000 / 168 |
| `usb_awake` | Stay awake while a computer is connected | `true` |
| `pin` | Bluetooth PIN (write-only) | up to 8 characters / none |
| `mesh` | Watch-to-watch messaging on | `false` |
| `mesh_group` | Group name — must match on every watch in the group | `class` |
| `mesh_ch` | Wi-Fi channel the group uses — must match | 1–13 / 1 |

Battery cost to keep in mind: each bell, log sample and time sync wakes the watch for a moment. Logging every minute or syncing every hour will shorten battery life noticeably; every 10–15 minutes and weekly are gentle.

## Where the time comes from

`time_src` picks the provider.

- **`ntp`** (default) — standard network time from the server in `ntp`.
- **`http`** — the watch fetches `time_url` and reads Unix time from the JSON field named in `time_key`. Seconds and milliseconds are both understood. Dots reach into nested objects: with `{"data":{"epoch":1767225600}}` use `data.epoch`. HTTPS works, but the watch does not verify the server's certificate (it has no certificate store) — fine for a clock, never send anything secret this way.

> **Open item.** The exact time/API service this project will use has not been decided yet. The `http` provider above is the general-purpose hook. If the chosen API needs something it cannot do — an API key header, a different response format, a POST — add a provider: one function in `firmware/watch_v1/timesource.h` plus one line in `timeFetch()`.

Without any network, a watch gets the time from the buttons, from *Set time from this computer*, or from another watch in its mesh group.

## Watch-to-watch (mesh)

Watches with the same `mesh_group` and `mesh_ch` talk directly by radio (ESP-NOW): no router, no pairing, roughly classroom-to-corridor range.

- The **mesh** screen shows how many group members are in range; SET buzzes them all.
- From Watch Studio (Settings → mesh) you can send four characters to every watch in the group.
- A watch that does not know the time copies it from one that does.

Limits, stated plainly:

- A sleeping watch hears nothing. The radio is on only while the mesh screen is showing (up to 2 minutes), in link mode, or while plugged into a computer. A buzz reaches the watches that are awake and listening at that moment.
- Messages go to every awake watch in range that has the same group name. The group name keeps other groups' traffic out; it is not encryption, so do not send anything private.
- This is one-hop broadcast, not a routed mesh network. A true always-on mesh (ESP-WIFI-MESH, painlessMesh) needs the radio on all the time, which would empty this battery in about two hours — it is only realistic on USB power. It is on the project to-do list.

## The command protocol

For scripts, other tools, or curiosity. The same commands work over USB and Bluetooth.

- **USB**: serial port, 115200 baud. **Bluetooth**: Nordic UART service `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`; write to `…0002`, replies arrive as notifications on `…0003` in 20-byte pieces.
- Send one JSON object per line. The watch answers with one JSON object per line: `{"ok":true,…}` or `{"ok":false,"err":"…"}`.
- Add any `"id"` and it is echoed in the reply.
- Lines that do not start with `{` are debug chatter; ignore them. Lines with an `"ev"` field are events (live data, button presses), not replies.

| Command | Fields | Does |
|---|---|---|
| `info` | | Firmware version, name, battery, time, clock chip, peers. Works while locked. |
| `auth` | `pin` | Unlock a Bluetooth session |
| `get` | | Returns all settings as `cfg` (no passwords) |
| `set` | `cfg` | Change any of the settings above; only the keys you send change |
| `wifi_add` | `ssid`, `pass` | Save or update a network |
| `wifi_del` | `ssid` | Forget a network |
| `wifi_scan` | | List networks in range |
| `sync` | | Fetch the time now |
| `time` | `epoch` | Set the clock (Unix seconds, UTC) |
| `env` | | One sensor reading |
| `log_get` | `from` | Up to 40 log rows from index `from`, plus `total`. Rows are `[unix time, °C, % humidity, hPa, light mV]`; `null` = no reading. |
| `log_clear` | | Empty the log |
| `buzz` | `ms` | Vibrate |
| `show` | `text`, `ms` | Show four characters |
| `mesh_send` | `text` | Send four characters to the group; without `text`, a buzz |
| `fs_info` | | File space `total` and `used` in bytes, number of `apps` |
| `fs_list` | `path` | `files`: `[{name, size, dir}]` in that folder |
| `fs_read` | `path`, `offset` | Up to a few hundred bytes from `offset`: `data`, `size`, `eof` |
| `fs_write` | `path`, `data`, `append` | Write (or with `append`, add to) a file. Send big files in pieces |
| `fs_delete` | `path` | Delete a file |
| `i2c_scan` | | `found`: addresses answering on the accessory socket's I²C bus |
| `i2c_read` | `addr`, `reg`, `len` | Read `len` (1–32) bytes, after writing register `reg` if given: `data` |
| `i2c_write` | `addr`, `data` | Write a list of bytes |
| `stream` | `ms` | Send `{"ev":"data",…}` lines every `ms` (100–60000; 0 stops) with sensor and battery readings. While streaming, button presses arrive as `{"ev":"btn","b":"mode"/"set","t":"short"/"long"}` |
| `reboot` | | Restart |
| `factory_reset` | | Erase all settings and restart |

Example session:

```
> {"cmd":"info"}
< {"ok":true,"fw":"2.1.0","name":"Watch","battery_pct":87,"time_valid":true,...}
> {"cmd":"wifi_add","ssid":"Staffroom","pass":"correct horse"}
< {"ok":true}
> {"cmd":"set","cfg":{"name":"Ms Rivera","views":["time","period","timer","temp"],"alerts":["08:30","09:25"]}}
< {"ok":true}
```

## Giving every watch the same starting point

Factory defaults live in `firmware/watch_v1/config.h` (time zone, default names). To pre-load a Wi-Fi network into freshly flashed watches, copy `secrets.example.h` to `secrets.h` and fill it in before uploading; it is only used when a watch has no saved settings, and the file is git-ignored.
