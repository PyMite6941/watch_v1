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
| **Screen Pack** (OLED clock faces, words, pictures) | 0.91" 128×32 SSD1306 I²C OLED + Screen Pack strip (+ the printed screen clip) | $0.99–1.09 ([AliExpress](https://www.aliexpress.com/i/2251832650376908.html)) | **$1.40–1.50** | Yes (fw 2.4.0; clock faces 2.5.0) |
| Screen Pack, larger | 0.96" 128×64 SSD1306 I²C OLED, on a cable (too tall for the strip) | not looked up yet | — | Yes |
| **Environment Pack** (temperature, humidity, pressure, logging) | BME280 breakout + Pack Base | $1.09–4.36 ([AliExpress](https://www.aliexpress.com/item/1005001621866431.html), [search](https://www.aliexpress.com/w/wholesale-bme280.html)) | **$1.50–4.80** | Yes |
| Environment, cheaper sensor | AHT20 + BMP280 combo board + Pack Base | $0.80–1.80 ([AliExpress](https://www.aliexpress.us/item/3256805134962180.html), [10-pack](https://www.aliexpress.us/item/3256809365206177.html)) | **$1.20–2.20** | Not yet: needs an AHT20 driver |
| **Light Pack** (lux meter) | BH1750 (GY-302) + Pack Base | about $1.09 ([AliExpress](https://www.aliexpress.com/w/wholesale-bh1750-sensor-module.html)) | **about $1.50** | Not yet: needs a BH1750 driver |
| **Sound Pack** (tuner) | MAX4466 microphone board + Pack Base | $2.50–2.95 ([Smart-Prototyping](https://www.smart-prototyping.com/MAX4466-Electret-Microphone-Amplifier-Module), [ProtoSupplies](https://protosupplies.com/product/max4466-microphone-preamplifier-module/)) | **$2.90–3.40** | Yes |

## Buying the parts in Thailand (Lazada)

Checked on lazada.co.th on 17 November 2026, in Thai baht. For each part I chose the listing that ships **from inside Thailand** and has the **most sales and reviews**, from a shop rated 98% or higher, rather than the cheapest listing. The cheapest listings usually ship from overseas with no sales history. Shipping is per shop: Lazada showed **฿29–45 standard delivery** to its default Bangkok address, so buy from as few shops as you can.

| Module | Part to buy | Price | Shop (rating, sales) | Works now? |
|---|---|---|---|---|
| **Screen Pack** | 0.91" 128×32 I²C OLED, white ([listing](https://www.lazada.co.th/products/pdp-i5196772066.html)) | **฿71**\* | BP Smart Shop (100%, 15.5K sold; this listing 416 sold, 58 reviews) | Yes |
| Screen Pack, larger | 0.96" 128×64 I²C OLED (same listing, 0.96" option) | not confirmed\* | same | Yes |
| **Environment Pack** | BME280 breakout, 5V or 3.3V ([listing](https://www.lazada.co.th/products/pdp-i16170541579.html)) | **฿115** | myElectron (99%, 683 sold) | Yes |
| Environment, cheaper | AHT20 + BMP280 board, option "AHT20 + BMP280_5V" ([listing](https://www.lazada.co.th/products/pdp-i4457501546.html)) | **฿50** | Karin Bot32 (98%, 5.8K sold, 4 years) | Needs an AHT20 driver |
| **Light Pack** | BH1750 / GY-302 ([listing](https://www.lazada.co.th/products/pdp-i1879688467.html)) | **฿53.59** | AEI.th (99%, 97.1K sold, 7 years) | Needs a BH1750 driver |
| **Sound Pack** | GY-MAX4466 microphone board ([listing](https://www.lazada.co.th/products/pdp-i5798998082.html)) | **฿55** | โอ.อาร์. เทคโนโลยี (99%, 288.1K sold) | Yes |
| Headers, every module | 40-pin 2.54 mm strips, ฿11 each ([listing](https://www.lazada.co.th/products/pdp-i6046877965.html)); 2×40 male, ฿16 ([listing](https://www.lazada.co.th/products/pdp-i326790101.html)) | about **฿3 a module** | เซอร์กิตทูเดย์ (99%, 57.9K sold) | — |

\* The search results showed ฿71 for this listing, but the product page opened on its 1.3" option (฿131) and stopped loading before I could select 0.91". Check the price for the 0.91" option before buying. Other 0.91" listings shipping from Thailand were ฿75–90 with no sales yet.

**Per module, parts only** (sensor or screen + about ฿3 of headers, without the board and shipping):

| Module | Parts | Cheapest reliable choice |
|---|---|---|
| Screen Pack | about **฿74** | the cheapest add-on that works today |
| Light Pack | about **฿57** | once the BH1750 driver exists |
| Environment Pack | about **฿118** with the BME280, about **฿53** with the AHT20 + BMP280 | the AHT20 board halves the cost once it has a driver |
| Sound Pack | about **฿58** | |

**Not chosen, and why:**
- ฿20 on the AHT20 listing is its bare BMP280 option, which has no humidity sensor.
- The ฿28–30 "MAX4466 sound detection sensor" listings may be a different board.
- The ฿48–65 "BME280" listings ship from overseas with almost no sales. Cheap "BME280" listings are often BMP280s (see below).

AliExpress (the table above) is cheaper per piece but ships from China and takes longer. Lazada is the reliable choice when you need a few parts quickly with an easy return (all of these listings offered 7-day free returns, no warranty).

## Boards from Elecrow instead of JLCPCB

[Elecrow](https://www.elecrow.com/pcb-manufacturing.html) also makes the boards: 2 layers, minimum 5, default 1.6 mm, green, HASL, normal lead time 4–7 working days. Its quote page only shows a price after you fill in the form, so **no price here**. Its home page advertises boards up to 10 × 10 cm "for $1", without saying how many boards that covers. Upload the Gerber zip from [ORDERING.md](ORDERING.md) to both Elecrow and JLCPCB with Thailand as the country, then compare the totals *with shipping*.

[Elecrow assembly](https://www.elecrow.com/pcb-assembly.html) is not worth it for these boards. They only carry pin headers, which take minutes to solder by hand, and the example quote alone shows a USD 12 stencil.

**Check before ordering BME280s.** The cheapest "BME280" listings are often the BMP280, which looks the same but has no humidity sensor. A real BME280 chip is square (about 2.5 × 2.5 mm). A BMP280 is longer than it is wide. The watch works with either, but with a BMP280 it never shows humidity.

## One whole watch, with a screen

Every watch gets a Screen Pack: a 0.91" OLED on a strip that lies on the lid, held by the printed screen clip ([hardware/addons](../hardware/addons/README.md#the-screen-pack-board)). Rough figures per watch, for a batch of about five, **before shipping and import tax**:

| Item | USD | Where the figure comes from |
|---|---|---|
| Watch board, 58 × 44 mm | under 1 | "$2 for 5" class offers (JLCPCB above); Elecrow not quoted, see below |
| Screen Pack strip, 40.5 × 10.9 mm | about 0.40 | the same offer; a second design is a second lot of 5 |
| 0.91" OLED | 0.99–1.09 (AliExpress) or ฿71 (Lazada, ships from Thailand) | tables above |
| Headers for the strip (2×3 male; the OLED's own 4-pin header comes with it) | a few cents (about ฿3) | Lazada strip prices above |
| Watch parts: SuperMini, display, chips, clock module, charger, battery, motor | 8–12 | [ORDERING.md](ORDERING.md#rough-cost-per-watch) |
| Passives, switches, LED, socket, screws | 2 | the same |
| Printed parts: base, lid, LED dome and spacer, screen clip | 1 (own printer) to 8 (ordered resin) | the same; sizes below |
| Strap | 2–4 | the same |
| **Total** | **about 15–30** | the parts list dominates; the screen adds about 1.50–2.50 |

**What gets printed, and how much material.** Measured from the STL files in `enclosure/stl/` (solid volume; a print service quoting by material weight works from this):

| Part | Volume |
|---|---|
| `base.stl` | 15.7 cm³ |
| `lid.stl` (or a patterned lid) | 4.5 cm³ |
| `screen_clip.stl` | 5.2 cm³ |
| `screen_clip_test.stl` (print once, to check the fit) | 2.3 cm³ |
| `led_dome.stl` + `led_spacer.stl` | 0.2 cm³ |
| **All five** (without the test piece) | **25.6 cm³** |

### Elecrow: the quote is still to get

Elecrow makes both the boards ([PCB](https://www.elecrow.com/pcb-manufacturing.html)) and the printed parts ([3D printing](https://www.elecrow.com/3d-printing-service.html), quoted by material weight from the uploaded STL), so a whole watch could come in one parcel. **Neither price is here.** Both quote pages only show a price once the form is filled in, and on 17 November 2026 the PCB page froze the browser twice while loading its form, before any price showed. To get the quote yourself:

1. **Boards:** on the PCB page enter 2 layers, 1.6 mm, HASL, then the size and quantity of each design: watch board **58 × 44 mm**, Screen Pack strip **40.5 × 10.9 mm**, Pack Base (size in [ORDERING.md](ORDERING.md)). Pick 5 or 10, and Thailand for shipping. Uploading the Gerber zip fills in the size by itself.
2. **Printed parts:** on the 3D-printing page upload `base.stl`, `lid.stl`, `screen_clip.stl`, `led_dome.stl` and `led_spacer.stl`, and pick the material.
3. Put the same files into JLCPCB / JLC3DP with Thailand as the country and compare the **totals with shipping**: for one small parcel, shipping decides it, not the board price.


## Keeping it cheap

1. **The Screen Pack is the cheapest way to give a teacher something new:** about $1.50 a module, and the clock faces, schedules and pictures cost nothing more.
2. **Buy screens and sensors in 5- or 10-packs.** Several listings above are cheaper per piece in packs.
3. **One basket, one parcel:** watch boards, Pack Bases and Screen Packs from JLCPCB together, so you pay shipping and any import tax once.
4. **The AHT20 + BMP280 board would be the cheapest environment sensor** once it has a driver (on the to-do list). Until then, use the BME280.
5. **Perfboard still works** for the sensor packs ([ADDONS.md](ADDONS.md)) and costs less than a board order if you only need one or two.
