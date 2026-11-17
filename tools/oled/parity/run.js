// OLED parity test: the watch (C) and Watch Studio (JS) must draw the same pixels.
//
//   node run.js make            writes cases.json: every built-in screen and starter with
//                               several sets of field values, plus seeded random designs
//   node run.js check <out.txt> compares parity.cpp's output with app/oled.js
//
// .github/workflows/oled-parity.yml runs both around the C++ build.
"use strict";
const fs = require("fs"), path = require("path");
const ROOT = path.join(__dirname, "..", "..", "..");
global.window = {};
eval(fs.readFileSync(path.join(ROOT, "app", "oled-assets.js"), "utf8"));
const OLED = require(path.join(ROOT, "app", "oled.js"));
const CASES = path.join(__dirname, "cases.json");

const FIELD_SETS = [
  { time: "10:42", hh: "10", mm: "42", ampm: "AM", day: "Monday", dy: "Mon", dd: "16", mon: "Nov", month: "November",
    batt: "76", charging: "1", wx_temp: "31°C", wx: "Partly cloudy", wx_icon: "partly", hi: "33", lo: "25", rain: "40",
    bell: "12:05", sec_pct: "28", step_pct: "60", step: "2/5", name: "Room 12", title: "LAB", icon: "clock",
    value: "10:42", label: "Monday 16 Nov", date: "Mon 16 Nov", greet: "Good morning", tick: "1", blink: "1",
    hh_math: "2^2+6", mm_math: "84/2", hh_bin: "1010", mm_bin: "101010", hh_hex: "0A", mm_hex: "2A", hh_rom: "X", mm_rom: "XLII" },
  { time: "--:--", batt: "3", charging: "0", wx_temp: "", wx_icon: "cloud", bell: "", icon: "thermo",
    value: "23.4°C", label: "23.4°C  48% humidity  1012 hPa", name: "" },
  { time: "23:59", batt: "100", icon: "chat", value: "QUIZ", title: "QUIZ", sec_pct: "100", step_pct: "-5",
    label: "Quiz on Friday: chapters 4 and 5, bring a calculator and a pencil. No phones!", name: "Café über ✓" },
];

// Small seeded generator, so the random cases are the same on every run.
let seed = 12345;
const rnd = n => { seed = (seed * 1103515245 + 12345) & 0x7fffffff; return seed % n; };
const pick = a => a[rnd(a.length)];
function randomItem() {
  const k = pick(["text", "text", "text", "icon", "battery", "bar", "box", "line", "circle", "bmp"]);
  const it = { k, x: rnd(160) - 16, y: rnd(80) - 8 };
  if (rnd(4) === 0) it.c = 0;
  if (rnd(6) === 0) it.if = pick(["wx_temp", "!wx_temp", "bell", "!charging", "nope"]);
  if (k === "text") {
    it.s = pick(["{time}", "{label}", "{value} {unknown}", "Hi {name}!", "  spaced   words here  ", "{", "{}", "x{ti me}", "", "°°"]);
    it.f = rnd(2) ? rnd(9) - 1 : [rnd(7), rnd(7), 0];
    if (rnd(2)) it.a = pick(["l", "c", "r", "z"]);
    if (rnd(2)) it.mw = rnd(140);
    if (rnd(3) === 0) it.wrap = rnd(5);
    if (rnd(4) === 0) it.lh = rnd(20);
    if (rnd(3) === 0) it.u = pick([1, true, 0, "yes"]);
  } else if (k === "icon") {
    it.n = pick(["clock", "{wx_icon}", "{icon}", "bee", "nope"]);
    it.z = pick([8, 16, 32, 12]);
  } else if (k === "bmp") {
    it.w = rnd(40); it.h = rnd(20);
    const bytes = new Uint8Array(((it.w + 7) >> 3) * it.h + rnd(3));
    for (let i = 0; i < bytes.length; i++) bytes[i] = rnd(256);
    it.d = OLED.toB64(bytes);
    if (rnd(4) === 0) it.d = it.d.slice(0, it.d.length >> 1) + " !*" + it.d.slice(it.d.length >> 1);
  } else {
    it.w = rnd(140) - 6; it.h = rnd(70) - 4; it.r = rnd(12) - 2; it.x2 = rnd(160) - 16; it.y2 = rnd(80) - 8;
    it.fill = pick([0, 1, true, false]); it.o = rnd(2);
    it.v = pick(["{sec_pct}", "{batt}", "{step_pct}", "150", "abc"]);
  }
  if (rnd(10) === 0) it.x = 3.7;
  return it;
}

function makeCases() {
  const cases = [];
  const add = (w, h, design) => FIELD_SETS.forEach(fields => cases.push({ w, h, fields, design }));
  for (const [size, set] of Object.entries(OLED.BUILTIN)) {
    const [w, h] = size.split("x").map(Number);
    for (const d of Object.values(set)) add(w, h, d);
  }
  for (const [size, list] of Object.entries(OLED.STARTERS)) {
    const [w, h] = size.split("x").map(Number);
    for (const d of list) add(w, h, d);
  }
  for (let i = 0; i < 300; i++) {
    const [w, h] = pick([[128, 32], [128, 64], [64, 32]]);
    const items = [];
    for (let j = 1 + rnd(6); j > 0; j--) items.push(randomItem());
    cases.push({ w, h, fields: pick(FIELD_SETS), design: { v: 1, w, h, inv: rnd(8) === 0, items } });
  }
  fs.writeFileSync(CASES, JSON.stringify(cases));
  console.log("wrote", cases.length, "cases");
}

function check(outFile) {
  const cases = JSON.parse(fs.readFileSync(CASES, "utf8"));
  const lines = fs.readFileSync(outFile, "utf8").trim().split(/\r?\n/);
  let bad = 0;
  cases.forEach((c, i) => {
    const g = OLED.render(c.design, k => c.fields[k], c.w, c.h);
    const want = Buffer.from(OLED.toBytes(g)).toString("hex");
    if (lines[i] !== want) {
      if (bad++ < 5) console.log("case", i, "differs:", JSON.stringify(c.design).slice(0, 300));
    }
  });
  console.log(bad ? `FAIL: ${bad} of ${cases.length} cases differ` : `ok: all ${cases.length} cases match`);
  process.exit(bad ? 1 : 0);
}

if (process.argv[2] === "make") makeCases();
else if (process.argv[2] === "check") check(process.argv[3]);
else console.log("usage: node run.js make | check <out.txt>");
