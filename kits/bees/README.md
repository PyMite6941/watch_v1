# Kit: biology, for someone who keeps (or loves) bees

## What to make

| Part | File / source | Notes |
|---|---|---|
| Watch | [`docs/BUILD_GUIDE.md`](../../docs/BUILD_GUIDE.md) | Standard build |
| Lid | `enclosure/stl/lid_honeycomb.stl` | Or set `lid_pattern = "honeycomb"` in `enclosure.scad` |
| Base | Export with `base_text`, such as a short line about bees or the year | Optional |
| Environment pack | GY-BME280 on a Pack Base ([`hardware/addons/`](../../hardware/addons/README.md)) | Temperature, humidity, pressure, and the sample log. **Works now** |
| Hive probe (optional) | A second BME280 on about 1 m of 4-core cable, inside `enclosure/stl/probe_sleeve.stl` + `probe_plug.stl` | See below |
| Sound pack (optional) | MAX4466 on a Pack Base | For the wingbeat activity. **Works now** (tuner screen) |
| Desk stand | `enclosure/stl/desk_stand.stl` | Keeps the watch upright on a bench |

## The preset

Load `preset.json` with Watch Studio › Backup › Restore from a backup file….

- **Screens:** time, date, temperature, weather (the forecast, from Wi-Fi), humidity, pressure, timer (10 min), battery. Weather needs a place set in Watch Studio › Settings › Weather.
- **Log** a sample every 30 minutes. Download it as a spreadsheet from Watch Studio.
- **Apps:**
  - `InSP`: a hive inspection sequence: suit up (2 min), light the smoker (1 min), open (1 min), check frames (10 min), close (2 min). It buzzes at each step, so there's no need to look at a clock with gloves on.
  - `bdAY`: days to World Bee Day (20 May). **Change the year each year.**
  - `bEE`: scrolls "bUSY AS A bEE".

## Ideas for using it

**Inside the hive.** A strong colony keeps its brood nest at about 34–35 °C whatever the weather outside. A probe in the hive logging every 30 minutes shows it:

- the steady line while the colony is healthy;
- the brood temperature dropping as winter comes;
- the clustering in winter.

For comparison, log the outside air too, on a second watch or for a week before the probe goes in.

- **Wiring the probe:** run 4 cores (3V3, GND, SDA, SCL) from a Pack Base's Qwiic or header pads to the BME280. A metre of cable is fine at the watch's I²C speed.
- **One sensor per watch:** the watch reads the first BME280 it finds, at `0x76` or else `0x77`. Use the probe *instead of* the pack on the watch, not as well. For inside and outside readings at the same time, log on two watches.
- **The sleeve:** the board slides in cable-last. The plug is split, so it closes round a cable that's already soldered. Its vent slots are 1.2 mm wide, too narrow for a bee. PETG survives a hive better than PLA.
- **Check with the beekeeper before putting anything in a hive.** Propolis will glue the probe in. The hive is warm and damp, so expect the sensor's humidity reading to drift after weeks inside.

**Wingbeats.** A honeybee's wings beat roughly 200–250 times a second. That is a low hum, and the tuner screen (sound pack) reads it as a note, around B♭3 (233 Hz). Students can:

- compare a worker with a drone;
- compare a bee with a fly (higher) or a bumblebee (lower);
- hum the note back.

Treat the readings as rough. The tuner is built for steady instrument notes, and a buzzing insect moves.

**Without bees.** The same pack and log suit any enclosed-climate lab:

- a terrarium;
- germinating seeds in a bag versus open air;
- a compost bin's heat.
