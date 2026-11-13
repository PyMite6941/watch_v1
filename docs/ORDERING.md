# Ordering boards and cases

Researched November 2026. Prices and customs rules move; treat the figures as a guide and get a live quote before paying.

## Do not order yet if…

- the board is still unrouted (`python hardware/scripts/export_fab.py` refuses to make Gerbers until it is), or
- no watch has been proven on a breadboard, or
- you have not measured your actual battery and charger board against the case.

A first order of five boards is the cheapest mistake-finder there is. Order five, build one, fix what is wrong, then order for everyone.

## The PCB

### What to upload

```bash
python hardware/scripts/export_fab.py
```

writes `hardware/fab/watch_v1_gerbers.zip`. Upload that one file.

### Order settings

| Setting | Value |
|---|---|
| Layers | 2 |
| Size | 58 × 44 mm (read from the file) |
| Quantity | 5 (the minimum everywhere) |
| Thickness | 1.6 mm — the case is designed around it |
| Material | FR-4 |
| Surface finish | HASL with lead is cheapest and solders easiest; lead-free HASL or ENIG if you prefer |
| Copper | 1 oz |
| Solder-mask colour | Any. The case is clear, so this is the watch's colour: black, white or purple look good behind an LED display. Green is usually fastest. |
| Min track / spacing | 0.2 / 0.2 mm, vias 0.7 / 0.35 mm — inside every factory's standard service |
| "Remove order number" | Worth the small fee, or choose "specify a location" — otherwise the factory prints a number somewhere visible |

### Where

| Factory | Good for | Notes |
|---|---|---|
| [JLCPCB](https://jlcpcb.com/) | Lowest price | Advertises from $2 for five 2-layer boards up to 100 × 100 mm; the real cost is shipping. One June 2026 notice mentions price adjustments for raw materials. Has its own 3D-printing arm, so boards and cases can share a parcel. |
| [PCBWay](https://www.pcbway.com/) | Service, special options | Usually a few dollars more for the same board; good support if a file is questioned. Also prints. |
| A local board house or university fab lab | Speed, no customs | Worth asking a school or university workshop: a single-evening turnaround beats two weeks of post. |

### Shipping and customs — Thailand

- Thailand **ended its duty-free allowance for low-value parcels on 1 January 2026**. Previously parcels up to THB 1,500 came in free of duty; now every import is assessed for VAT and duty on its declared value.
- Expect 7 % VAT plus any duty for the tariff code, collected by the courier before delivery, often with a handling fee. The rate for bare circuit boards was not confirmed in this research — check at the factory's checkout (some offer duty-paid shipping, which removes the surprise) or with Thai Customs.
- Combine everything into as few parcels as possible: handling fees are per parcel.
- Typical timing: 2–3 days to make, then 3–5 days by express courier or 1–3 weeks by economy post.

## The case

Two printed parts, `base.stl` and `lid.stl`, in `enclosure/stl/`. Print `fit_test.stl` first.

### Print it yourself, or locally

Any ordinary filament printer can make it: no supports, 0.2 mm layers. About 30 g of filament for the pair.

- **Clear PETG** gives a frosted see-through — parts are visible, edges are soft. Tough, slightly flexible, the practical choice.
- Printing tips for clarity: 0.2 mm layers or thicker, 100 % infill for the lid, nozzle at the hot end of the range, fan low, slow speed.

### Order it

| Service | Material to choose | Notes |
|---|---|---|
| [JLC3DP](https://jlc3dp.com/) | SLA transparent resin (listed as "8001") | No minimum order; advertised from $0.30 per part, a watch case will be a few dollars. Tolerance about ±0.2 mm. Ships with a JLCPCB board order. |
| [PCBWay](https://www.pcbway.com/) 3D printing | SLA transparent resin (UTR-8100) with **spray varnish** finish | The varnish is what turns frosted resin glass-clear. |
| A local print shop / makerspace | Clear PETG, or clear resin | No shipping or customs; you can hand over the fit-test piece first. |

Resin versus PETG for this watch:

| | Clear resin (SLA) | Clear PETG (filament) |
|---|---|---|
| Look | Close to glass, especially varnished | Frosted |
| Detail | Sharp | Visible layer lines |
| Toughness | Brittle — can crack if dropped or if screws are overtightened | Tough |
| Ageing | Yellows in sunlight over months | Stable |
| Screws | Use M2 heat-set inserts or go gently; set `screw_tap_d` accordingly | Self-tapping is fine |

For a gift that will be worn daily, **PETG for the base and resin for the lid** is a good mix: toughness where the screws and strap pull, clarity where people look.

Resin parts come ±0.2 mm; the model has 0.4 mm of clearance around the PCB, so they should fit, but that is exactly what the fit test is for.

## Parts

For Thailand: the modules and passives are all stocked by local sellers on Shopee and Lazada and by Thai hobby shops, which avoids import handling entirely. Search the exact part names from [BOM.md](BOM.md). Three to check carefully before buying:

1. the display is **common cathode**;
2. the transistors are **E-B-C**;
3. the tactile switches are **13 mm tall**.

For a batch of watches, buying resistors, transistors and capacitors in packs of 50–100 costs barely more than buying ten.

## Rough cost per watch

Order-of-magnitude only, excluding shipping and tax, for a batch of about five:

| Item | USD |
|---|---|
| PCB | under 1 |
| SuperMini, display, chips, clock module, charger, battery, motor | 8–12 |
| Passives, switches, LED, socket, screws | 2 |
| Printed case | 1 (own printer) to 8 (ordered resin) |
| Strap | 2–4 |
| Each pack | 2–4 |

Shipping and import handling for the board order will likely exceed the cost of the boards themselves.

## Sources

- [JLCPCB — how to place an order, pricing basics](https://jlcpcb.com/help/article/How-do-I-place-an-order)
- [JLCPCB — manufacturer comparison page](https://jlcpcb.com/ads/pcb-manufacturer-comparison)
- [JLCPCB news (price adjustment notices)](https://jlcpcb.com/news)
- [JLC3DP — professional 3D printing, materials and starting prices](https://jlc3dp.com/events/professional-3d-printing)
- [Adafruit — clear resin 3D prints with PCBWay](https://learn.adafruit.com/clear-resin-3d-prints-with-pcbway?view=all)
- [3D Print Beginner — JustWay vs PCBWay vs JLCPCB](https://3dprintbeginner.com/justway-vs-pcbway-vs-jlcpcb-what-to-use/)
- [All3DP — best cheap 3D printing services](https://all3dp.com/2/best-cheap-3d-printing-services/)
- [Tilleke & Gibbins — Thailand customs duty changes, end of the THB 1,500 exemption](https://www.tilleke.com/insights/thailand-announces-customs-duty-hikes-for-key-consumer-goods)
