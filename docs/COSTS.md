# What each add-on module costs

Estimates for one module, in US dollars, from online listings checked in November 2026. They are **snapshots**: AliExpress prices move every week, and many listings show a crossed-out "standard" price next to the real one. Get a live price before buying many.

Shipping is not included. JLCPCB and AliExpress work it out from the parcel's weight and your country, and neither publishes a rate for Thailand. Put everything in as few parcels as possible ([ORDERING.md](ORDERING.md)).

## The boards

| Item | Price | Source |
|---|---|---|
| A bare add-on board (Pack Base or Screen Pack), 2 layers, under 100 × 100 mm | from **$2 for 5**, so about **$0.40 each** | [JLCPCB](https://jlcpcb.com/) home page offer; the price depends on the options chosen |
| Pin headers and sockets (2×3 male, 1×4 female) | **not looked up**: they are sold in strips and usually cost cents | — |
| A printed holder or cover, SLA resin | from **$0.30 a part**, no minimum | [JLC3DP](https://jlc3dp.com/) price floor; real parts cost more with size |

Each board design is its own minimum order of 5, so a Pack Base order and a Screen Pack order are two lots of "$2 for 5".

## Per module

"Total" = the sensor or screen + one board at $0.40. It leaves out headers and shipping.

| Module | What you buy | Sensor / screen price | Total, about | Works now? |
|---|---|---|---|---|
| **Screen Pack** (OLED clock faces, words, pictures) | 0.91" 128×32 SSD1306 I²C OLED + Screen Pack board | $0.99–1.09 ([AliExpress](https://www.aliexpress.com/i/2251832650376908.html)) | **$1.40–1.50** | Yes (fw 2.4.0; clock faces 2.5.0) |
| Screen Pack, larger | 0.96" 128×64 SSD1306 I²C OLED | not looked up yet | — | Yes |
| **Environment Pack** (temperature, humidity, pressure, logging) | BME280 breakout + Pack Base | $1.09–4.36 ([AliExpress](https://www.aliexpress.com/item/1005001621866431.html), [search](https://www.aliexpress.com/w/wholesale-bme280.html)) | **$1.50–4.80** | Yes |
| Environment, cheaper sensor | AHT20 + BMP280 combo board + Pack Base | $0.80–1.80 ([AliExpress](https://www.aliexpress.us/item/3256805134962180.html), [10-pack](https://www.aliexpress.us/item/3256809365206177.html)) | **$1.20–2.20** | Not yet: needs an AHT20 driver |
| **Light Pack** (lux meter) | BH1750 (GY-302) + Pack Base | about $1.09 ([AliExpress](https://www.aliexpress.com/w/wholesale-bh1750-sensor-module.html)) | **about $1.50** | Not yet: needs a BH1750 driver |
| **Sound Pack** (tuner) | MAX4466 microphone board + Pack Base | $2.50–2.95 ([Smart-Prototyping](https://www.smart-prototyping.com/MAX4466-Electret-Microphone-Amplifier-Module), [ProtoSupplies](https://protosupplies.com/product/max4466-microphone-preamplifier-module/)) | **$2.90–3.40** | Yes |

**Check before ordering BME280s.** The cheapest "BME280" listings are often the BMP280, which looks the same but has no humidity sensor. A real BME280 chip is square (about 2.5 × 2.5 mm). A BMP280 is longer than it is wide. The watch works with either, but with a BMP280 it never shows humidity.

## Keeping it cheap

1. **The Screen Pack is the cheapest way to give a teacher something new:** about $1.50 a module, and the clock faces, schedules and pictures cost nothing more.
2. **Buy screens and sensors in 5- or 10-packs.** Several listings above are cheaper per piece in packs.
3. **One basket, one parcel:** watch boards, Pack Bases and Screen Packs from JLCPCB together, so you pay shipping and any import tax once.
4. **The AHT20 + BMP280 board would be the cheapest environment sensor** once it has a driver (on the to-do list). Until then, use the BME280.
5. **Perfboard still works** for the sensor packs ([ADDONS.md](ADDONS.md)) and costs less than a board order if you only need one or two.
