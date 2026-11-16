# Ordering boards and cases

Researched November 2026. Prices and customs rules move; treat the figures as a guide and get a live quote before paying.

## Do not order yet if…

- `hardware/fab/watch_v1_gerbers.zip` is missing or older than the last placement change (the cloud workflow in [`hardware/README.md`](../hardware/README.md) makes it), or
- no watch has been proven on a breadboard, or
- you have not measured your actual battery and charger board against the case.

A first order of five boards is the cheapest mistake-finder there is. Order five, build one, fix what is wrong, then order for everyone.

## The PCB

### What to upload

`hardware/fab/watch_v1_gerbers.zip`, made by the cloud routing workflow. Upload that one file. (No KiCad, Java or router needed on your computer.)

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
| Min track / spacing | 0.15 / 0.2 mm (the router narrows a few tracks next to pins), vias 0.7 / 0.35 mm — inside every factory's standard service (JLCPCB and PCBWay go down to 0.127 mm) |
| "Remove order number" | Worth the small fee, or choose "specify a location" — otherwise the factory prints a number somewhere visible |

### Where

| Factory | Good for | Notes |
|---|---|---|
| [JLCPCB](https://jlcpcb.com/) | Lowest price | Advertises from $2 for five 2-layer boards up to 100 × 100 mm; the real cost is shipping. One June 2026 notice mentions price adjustments for raw materials. Has its own 3D-printing arm, so boards and cases can share a parcel. |
| [PCBWay](https://www.pcbway.com/) | Service, special options | Usually a few dollars more for the same board; good support if a file is questioned. Also prints. |
| A local board house or university fab lab | Speed, no customs | Worth asking a school or university workshop: a single-evening turnaround beats two weeks of post. |

**Finding a service near you:** [find-a-printer.vercel.app](https://find-a-printer.vercel.app) lists PCB, 3D-printing, CNC and sheet-metal services with ratings gathered from several sources, certifications and contact links. Use it to find a nearby board house or print shop, or to compare the big online ones before ordering.

### Shipping and customs

Rules depend on the country you are shipping to, so check yours before paying:

- **Duty-free allowances for small parcels are shrinking.** Several countries have recently ended or cut the allowance that used to let low-value parcels in free, so even a $5 board order may now be charged VAT or sales tax and duty, often plus a courier handling fee.
- **Look for duty-paid shipping** at the factory's checkout. It removes the surprise bill on delivery.
- **Combine everything into as few parcels as possible.** Watch boards, Pack Bases and printed cases from the same company can share one parcel, and handling fees are per parcel.
- **Typical timing:** 2–3 days to make, then 3–5 days by express courier or 1–3 weeks by economy post.
- **A local service** found through [find-a-printer.vercel.app](https://find-a-printer.vercel.app) avoids customs entirely.

## The case

Four printed parts in `enclosure/stl/`: `base.stl`, `lid.stl`, `led_dome.stl` (the hexagonal light dome over the RGB LED) and `led_spacer.stl` (sets the LED's height). Print `fit_test.stl` first. Clearances are set for a first print to fit without sanding: 0.5 mm round the board, 0.3 mm round the lid's lip, 0.2 mm round the dome's stem, 0.3 mm round the LED.

Optional extras in the same folder: lids with an engraved pattern (`lid_honeycomb`, `lid_binary`, `lid_staff`, `lid_sine`), a desk stand (`desk_stand.stl`, about 25 g) and a vented sensor probe case (`probe_sleeve.stl` + `probe_plug.stl`). Which suits whom: [`kits/`](../kits/README.md).

### Print it yourself, or locally

Any ordinary filament printer can make it: no supports, 0.2 mm layers. About 30 g of filament for the pair.

- **Clear PETG** gives a frosted see-through — parts are visible, edges are soft. Tough, slightly flexible, the practical choice.
- Printing tips for clarity: 0.2 mm layers or thicker, 100 % infill for the lid, nozzle at the hot end of the range, fan low, slow speed.

### Order it

| Service | Material to choose | Notes |
|---|---|---|
| [JLC3DP](https://jlc3dp.com/) | SLA transparent resin (listed as "8001") | No minimum order; advertised from $0.30 per part, a watch case will be a few dollars. Tolerance about ±0.2 mm. Ships with a JLCPCB board order. |
| [PCBWay](https://www.pcbway.com/) 3D printing | SLA transparent resin (UTR-8100) with **spray varnish** finish | The varnish is what turns frosted resin glass-clear. |
| A local print shop / makerspace | Clear PETG, or clear resin | No shipping or customs; you can hand over the fit-test piece first. Find one on [find-a-printer.vercel.app](https://find-a-printer.vercel.app). |

Resin versus PETG for this watch:

| | Clear resin (SLA) | Clear PETG (filament) |
|---|---|---|
| Look | Close to glass, especially varnished | Frosted |
| Detail | Sharp | Visible layer lines |
| Toughness | Brittle — can crack if dropped or if screws are overtightened | Tough |
| Ageing | Yellows in sunlight over months | Stable |
| Screws | Use M2 heat-set inserts or go gently; set `screw_tap_d` accordingly | Self-tapping is fine |

For a gift that will be worn daily, **PETG for the base and resin for the lid** is a good mix: toughness where the screws and strap pull, clarity where people look.

Resin parts come ±0.2 mm; the model has 0.5 mm of clearance around the PCB, so they should fit, but that is exactly what the fit test is for.

## Parts

Every part is a common hobby part. Local electronics shops and local online marketplaces usually stock them, which avoids import handling entirely; otherwise AliExpress, LCSC or a national distributor. Search the exact part names from [BOM.md](BOM.md). Three to check carefully before buying:

1. the display is **common cathode**;
2. the transistors are **E-B-C**;
3. the tactile switches are **13 mm tall**.

For a batch of watches, buying resistors, transistors and capacitors in packs of 50–100 costs barely more than buying ten.

## In Bangkok

Researched November 2026, from web sources listed below. Nothing here has been ordered yet, so ring or email before going. Some sources are a few years old, and opening hours and machines change. Cheapest first:

### 1. Boards: still cheapest from China, but budget for tax

No Bangkok board house found advertises prototype prices anywhere near JLCPCB's or PCBWay's. Local assembly houses are aimed at production runs. For example, C.E. ANTS lists a minimum order of about US$2,000 for low-volume SMT assembly. So order the bare boards online and solder them yourself (every part on this watch is through-hole, except the optional Qwiic socket on the Pack Base).

**Customs:**
- Thailand ended its 1,500-baht duty-free allowance on 1 January 2026.
- Every imported parcel is now assessed for **7 % VAT plus any import duty**, whatever its value. VAT is charged on the value plus duty.
- The duty rate depends on the goods; check the rate for printed circuit boards when the quote arrives.

**Ways to keep the tax down:**
- Put the watch boards, Pack Bases and printed cases in **one parcel**.
- Prefer a checkout option that shows tax and duty paid up front, if the factory offers one for Thailand.

### 2. Printed case: a makerspace, or JLC3DP in the same parcel

| Where | What it has | Notes |
|---|---|---|
| **FabLab Bangkok** (Banglamphu) | 3D printers, laser cutters, CNC milling, circuit-board production | Its listing also names PCB assembly. Bring `fit_test.stl` first. The Electrical and Electronics Institute's (EEI) design centre runs small-lot SMT assembly from FabLab Bangkok's building (KX), which is the one local route to machine-assembled boards in small numbers. |
| **FabCafe Bangkok** (Central Post Office building, Charoen Krung) | 3D printers, laser cutters | Café-style: good for printing a case while you wait. Source is a travel FAQ and a 2017 article, so check it is still open. |
| **Fab Lab Siam** | 3D printing, CNC milling, circuit production, laser cutting | Listed on fablabs.io. |
| A makerspace and electronics shop at **Fortune Town** IT mall | Workshop and DIY electronics shop | Described in 2017-2018 articles; check it is still there. |
| [JLC3DP](https://jlc3dp.com/) | Clear resin or PETG | Ships with the board order, so it shares the same tax bill instead of adding a second parcel. |

No Bangkok per-gram prices were published. Ask for a quote with the STL attached: the base and lid are about 30 g together, and the dome and spacer are a few grams more.

### 3. Parts: Ban Mo

**Ban Mo** (near Phahurat, old town) is Bangkok's traditional electronics street: hundreds of shops selling components, LEDs, cables and chargers, with repair shops that also do hand soldering. A forum thread recommends it for soldering work and notes that it helps to bring a Thai speaker. Take [BOM.md](BOM.md) printed out, and check the three tricky parts listed above (common-cathode display, E-B-C transistors, 13 mm switches).

### 4. Assembly, if you would rather not solder

| Where | Fits this project? |
|---|---|
| EEI PCB assembly service (at FabLab Bangkok) | Small lots. Send Gerbers, drill file, pick-and-place, BOM and assembly drawing. The watch is all through-hole, so ask whether they hand-solder THT. |
| Surplus Technology (Bang Khen) | Prototype to production; quote by email. |
| Meesinsup Technology | Published prototype + assembly packages, 30-90 days, and you supply the boards, stencil and parts. Slower than soldering it yourself. |
| C.E. ANTS | Minimum about US$2,000: only for a large batch. |

For five gift watches, **solder them yourself** (an evening each with the build guide) or ask a Ban Mo repair shop. Both cost far less than any assembly house.

**Cheapest plan for a first copy:** boards, Pack Bases and resin lid from JLCPCB/JLC3DP in one parcel (one tax bill); the PETG base printed at FabLab Bangkok or a makerspace; parts from Ban Mo; solder it yourself.

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
- [find-a-printer — rated PCB, 3D-printing, CNC and sheet-metal services](https://find-a-printer.vercel.app)
- [EEI — PCB assembly service (FabLab Bangkok, KX building)](https://www.thaieei.com/en/services/electronic-design-services/electronic-circuit-board-assembly-service-pcb-assembly/)
- [C.E. ANTS — Bangkok SMT assembly, minimum orders](https://www.ceants.com/)
- [Surplus Technology — Bangkok](https://surplus.co.th/en)
- [Meesinsup — PCB assembly packages](https://meesinsup.com/pcb-assembly/)
- [FabLab Bangkok on fablabs.io](https://fablabs.io/labs/fablabbkk)
- [Fab Lab Siam on fablabs.io](https://www.fablabs.io/labs/fablabsiam?locale=en)
- [FabCafe Bangkok — visitor FAQ](https://yourthaiguide.com/fabcafe-bangkok-tourist-faq/)
- [Make: — Bangkok's growing maker movement](https://makezine.com/article/education/bangkoks-growing-maker-movement/)
- [Make: — a makerspace in a shopping mall](https://makezine.com/2018/07/13/makerspace-shopping-mall-makes-sense-world)
- [Ban Mo, Bangkok — Wikipedia](https://en.wikipedia.org/wiki/Ban_Mo,_Bangkok)
- [Thaivisa forum — hand soldering in Bangkok](https://forum.thaivisa.com/topic/1144223-hand-soldering-in-bangkok/)
- [Thailand ends duty relief for low-value imports, January 2026 (VATupdate)](https://www.vatupdate.com/2026/01/03/thailand-to-end-customs-duty-relief-for-low-value-imports-from-january-2026/)
- [Thailand customs duty 2026: end of de minimis (Lex Bangkok)](https://lexbangkok.com/thailand-customs-duty-2026/)
- [Bangkok Post — customs rule change trims parcel imports](https://www.bangkokpost.com/business/general/3226823/customs-rule-change-trims-parcel-imports)
