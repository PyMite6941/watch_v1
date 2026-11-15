# Kit: band

## What to make

| Part | File / source | Notes |
|---|---|---|
| Watch | [`docs/BUILD_GUIDE.md`](../../docs/BUILD_GUIDE.md) | Standard build |
| Lid | `enclosure/stl/lid_staff.stl` | Five staff lines with a few notes |
| Base | Export with `base_text`, such as the band's name | Optional |
| Sound pack | MAX4466 on a Pack Base ([`hardware/addons/`](../../hardware/addons/README.md)) | Needed for the tuner. **Works now** |
| Desk stand | `enclosure/stl/desk_stand.stl` | Stands on a music-stand ledge or a podium, so the timer or metronome faces the director |

## The preset

- **Screens:** time, tuner (A4 = 440 Hz), metronome (100 bpm), timer, stopwatch.
- **Apps:**
  - `UArn`: a warm-up sequence: breathing (1 min), long tones (2 min), scales (3 min), tuning (1 min). It buzzes at each change.
  - `SECt`: picks a section at random, with no repeats until every section has had a turn. Use it to choose who plays the passage alone next. Edit the list to match the band. Names are 4 seven-segment characters, so `FLUt`, `trPt` and `PErC` work, but anything with M or W won't.

## Ideas for using it

- **The silent metronome.** The watch taps the wrist on each beat instead of clicking. A director can feel the tempo during a rehearsal without an audible click, and a student can keep steady time in a practice room without disturbing the next room. Hold SET on the metronome to step through 60, 72, 80, 90, 100, 120, 140 and 160 bpm.
- **The tuner on the stand.** Put the watch on the desk stand and the sound pack facing the player. It shows note, octave and how far off: see [`docs/USER_GUIDE.md`](../../docs/USER_GUIDE.md) › *Reading the tuner*. Change `tune_a4` if the band tunes to 442.
- **Why does the note drift?** Leave a wind instrument cold, then warm it by playing. The pitch rises as the air inside warms, because the speed of sound depends on temperature. Add an environment pack and the watch can show the room temperature too.
- **Timed sectionals.** Change `UArn`, or make another sequence in Watch Studio › Apps. For example, 10 minutes each for woodwinds, brass and percussion, with a buzz at each change.
- **A concert countdown.** Add a countdown app in Watch Studio with the date of the next concert.
