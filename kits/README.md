# Kits: a watch set up for one person

A kit is a recipe for turning a standard Watch V1 into a gift for one particular person. It changes nothing in the firmware or the circuit, so anyone can make one, or make their own.

Each kit folder has:

| File | What it is |
|---|---|
| `README.md` | What to make, what to print, and ideas for using it |
| `preset.json` | Screens, settings and custom apps. Watch Studio has a copy of every kit: Apps › Kits › *Load onto the watch* |
| (sometimes) scripts | Extra things to do with the watch from a computer |

A kit picks from four things:

1. **A preset**: which screens appear, and custom apps (step timers, name pickers, countdowns, messages).
2. **A pack**: a sensor that plugs into the accessory socket. See [`hardware/addons/`](../hardware/addons/README.md).
3. **A personal case**: a pattern engraved into the lid and a line of text under the base. These are settings in [`enclosure/enclosure.scad`](../enclosure/enclosure.scad) › *Personal touches*.
4. **Printed accessories**: a desk stand, and a vented probe case for a sensor on a cable. See [`enclosure/accessories.scad`](../enclosure/accessories.scad).

## The kits

| Kit | Pack | Lid pattern | Accessories |
|---|---|---|---|
| [Everyday](everyday/README.md) | None needed | Any | Desk stand |
| [Biology: bees](bees/README.md) | Environment (BME280), optionally on a cable as a hive probe; sound pack for wingbeats | `honeycomb` | Probe sleeve + plug, desk stand |
| [Computer science](computer-science/README.md) | Any pack: the I²C scan and the serial protocol are the lesson. Python scripts included | `binary` | Desk stand |
| [Band](band/README.md) | Sound (MAX4466) for the tuner | `staff` | Desk stand (it fits a music-stand ledge) |
| [Calculus](calculus/README.md) | Environment (BME280): pressure gives height, so a derivative you can walk | `sine` | Desk stand |
| [Youth group](youth-group/README.md) | None needed | `none`, or text under the base | Desk stand |

## Making a kit's watch

1. Build the watch as usual ([`docs/BUILD_GUIDE.md`](../docs/BUILD_GUIDE.md)).
2. **Case:** open `enclosure/enclosure.scad` in OpenSCAD. In the Customizer, under *Personal touches*, pick the `lid_pattern` and type the `base_text`. Then export the lid and base. The `enclosure/stl/` folder already has a lid for each pattern (`lid_honeycomb.stl`, `lid_binary.stl`, `lid_staff.stl`, `lid_sine.stl`). Those lids have no text under the base, so export your own base if you want text there.
3. **Accessories:** the STLs are in `enclosure/stl/`: `desk_stand.stl`, `probe_sleeve.stl` and `probe_plug.stl`.
4. **Pack:** build it on a Pack Base ([`hardware/addons/`](../hardware/addons/README.md)). Remove the module's power LED first, or it will drain the battery.
5. **Preset:** connect the watch in Watch Studio, open **Apps › Kits** and press *Load onto the watch* on the kit. (Or **Backup › Restore from a backup file…** with the kit's `preset.json`.) If the watch would end up with more than 8 apps, it offers to remove the others first.
   - The preset only replaces the settings it lists, plus the apps it brings.
   - Name, time zone, Wi-Fi and PIN stay as they are, so run Quick setup as well.
   - Restoring writes the kit's apps. Apps already on the watch with other file names stay, up to the limit of 8.

No printer? [find-a-printer.vercel.app](https://find-a-printer.vercel.app) lists rated 3D-printing and PCB services near you. [`docs/ORDERING.md`](../docs/ORDERING.md) has the rest.

## Making your own kit

Copy a kit folder and edit its `preset.json`. It is an ordinary Watch Studio backup file:

```json
{
  "watch_backup": 1,
  "cfg": { "views": ["time", "date", "timer"], "timer_s": 300 },
  "files": {
    "/apps/break.json": "{\"type\":\"countdown\",\"title\":\"brEA\",\"date\":\"2027-06-20\"}"
  }
}
```

- `cfg` holds any settings from [`docs/CONFIGURATION.md`](../docs/CONFIGURATION.md). Leave out `name`, `tz`, `wifi` and `pin`: they belong to the owner.
- `files` holds the custom apps. Each one is a JSON string, as Watch Studio writes them. The four app types, `sequence`, `picker`, `countdown` and `message`, are described at the top of [`firmware/watch_v1/apps.h`](../firmware/watch_v1/apps.h). The presets here show one of each.
- An easier route: set the watch up by hand in Watch Studio, then use **Backup › Save a backup file**. Delete the personal keys from the file.

Then check it:

```
python kits/check_kit.py                 # every kit
python kits/check_kit.py my-kit/preset.json
python kits/build_gallery.py             # copy the kits into Watch Studio
```

A preset needs `title`, `about` and `needs` too: the gallery in Watch Studio shows them. `check_kit.py` also fails when Watch Studio's copy of the kits is out of date.

It catches what the watch would get wrong:

- **Text the display can't draw.** The display has seven segments, so K, M, V, W, X and Z come out blank. Upper/lower case changes the shape: `b` and `d` are lowercase only.
- **Titles longer than 4 characters.**
- **More than 8 apps.**
- **Bad dates** and **step lengths that don't fit.**

**Dates go stale.** Countdowns in these presets point at fixed days in 2027: Pi Day and World Bee Day. Change them each year, and add the person's own dates, such as exams, concerts and camps, in Watch Studio › Apps.
