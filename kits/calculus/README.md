# Kit: calculus

For a teacher of AP Calculus AB and BC. Uses the environment pack, because air pressure turns the watch into an altimeter, which gives a function of time to differentiate.

## What to make

| Part | File / source | Notes |
|---|---|---|
| Watch | [`docs/BUILD_GUIDE.md`](../../docs/BUILD_GUIDE.md) | Standard build |
| Lid | `enclosure/stl/lid_sine.stl` | One period of a sine wave |
| Base | Export with `base_text`, such as `dy/dx` | Optional |
| Environment pack | GY-BME280 or the cheaper BMP280, on a Pack Base ([`hardware/addons/`](../../hardware/addons/README.md)) | Pressure works with both. **Works now** |
| Desk stand | `enclosure/stl/desk_stand.stl` | FRQ timer facing the class |
| Script | `../computer-science/watch.py` | Live readings to a computer |

## The preset

- **Screens:** time, date, pressure, temperature, stopwatch, timer (15 min).
- **Apps:**
  - `FrQ`: six 15-minute steps, FrQ1 to FrQ6. That matches the free-response section's pace, 90 minutes for six questions. It buzzes when it's time to move on.
  - `CALL`: random number 1–30.
  - `PI`: days to Pi Day. **Change the year each year,** and add the exam date as another countdown in Watch Studio.

## Ideas for using it

**A derivative you can walk.** Near the ground, air pressure falls by about 0.12 hPa for every metre you climb, so a floor of a building (about 3 m) is roughly 0.36 hPa.

1. Stream the pressure to a laptop while a student walks up a flight of stairs, waits, and walks down:

   ```python
   import time
   from watch import Watch
   with Watch() as w:
       t0 = time.time()
       for r in w.stream(500):
           print(round(time.time() - t0, 1), r.get("hpa"))
   ```

2. Convert to height: h ≈ (p₀ − p) / 0.12 metres, where p₀ is the first reading.
3. Plot h(t). Estimate h′(t) with difference quotients, find where the climb was fastest, and compare the speeds going up and coming down. Integrate the velocity back to height and check it against the measured height.
4. Talk about the noise. Readings come in steps of 0.1 hPa, and the air drifts. That is why a difference quotient over 0.5 s is useless here, while one over 5 s works. That makes a good discussion about Δt.

**Related rates and modelling.**

- **Logging a day.** Log the classroom temperature every 10 minutes for a day. Fit a sinusoid, then ask: when is the room warming fastest?
- **The pressure trend before a storm.** Log for a week and look at dp/dt.

**Later (needs firmware drivers, see [`hardware/addons/`](../../hardware/addons/README.md)):**

- a distance sensor (VL53L0X) for position, velocity and acceleration graphs of a hand or a cart;
- a motion sensor (MPU6050) for a pendulum: a real sine to fit, and its derivative.
