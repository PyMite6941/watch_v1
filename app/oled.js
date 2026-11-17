// Draws OLED screen designs, pixel for pixel the same as the watch does.
//
// This is a line-by-line twin of firmware/watch_v1/oled_render.h: same shapes,
// same text fitting, same fonts (app/oled-assets.js). If one changes, change the
// other; tools/oled/parity/ checks they agree. The screen file format is in
// docs/OLED_SCREENS.md.
"use strict";
const OLED = (() => {
  const A = window.OLED_ASSETS;
  // Base64 -> bytes. Skips anything that is not a base64 character and stops at '=',
  // exactly as the watch's decoder does, so a damaged "bmp" draws the same on both.
  const B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  function b64(s) {
    const out = [];
    let acc = 0, n = 0;
    for (const ch of String(s || "")) {
      if (ch === "=") break;
      const v = B64.indexOf(ch);
      if (v < 0) continue;
      acc = (acc << 6) | v; n += 6;
      if (n >= 8) { n -= 8; out.push((acc >> n) & 0xFF); }
    }
    return Uint8Array.from(out);
  }
  const FONTS = A.fonts.map(f => f.classic ? { classic: true, name: f.name, bitmap: b64(f.bitmap) }
    : { name: f.name, first: f.first, last: f.last, yAdvance: f.yAdvance, bitmap: b64(f.bitmap), glyphs: Int16Array.from(f.glyphs) });
  const ICONS = {};
  for (const k in A.icons) ICONS[k] = b64(A.icons[k]);
  const DEG = 0xB0;

  // ------------------------------------------------------------- numbers
  // Whole numbers, cut to +-4096 so a typo cannot keep the watch drawing for seconds.
  const num = (v, d) => typeof v === "number" && isFinite(v) ? Math.max(-4096, Math.min(4096, Math.trunc(v))) : d;
  const truthy = v => typeof v === "boolean" ? v : num(v, 0) !== 0;
  const str = v => typeof v === "string" ? v : "";

  // --------------------------------------------------------------- canvas
  function canvas(w, h) { return { w, h, px: new Uint8Array(w * h) }; }
  function px(g, x, y, c) { if (x >= 0 && y >= 0 && x < g.w && y < g.h) g.px[y * g.w + x] = c; }
  function fillRect(g, x, y, w, h, c) { for (let j = 0; j < h; j++) for (let i = 0; i < w; i++) px(g, x + i, y + j, c); }
  function rect(g, x, y, w, h, c) {
    if (w <= 0 || h <= 0) return;
    for (let i = 0; i < w; i++) { px(g, x + i, y, c); px(g, x + i, y + h - 1, c); }
    for (let j = 0; j < h; j++) { px(g, x, y + j, c); px(g, x + w - 1, y + j, c); }
  }
  // Bresenham, as Adafruit GFX's writeLine.
  function line(g, x0, y0, x1, y1, c) {
    const steep = Math.abs(y1 - y0) > Math.abs(x1 - x0);
    if (steep) { [x0, y0] = [y0, x0]; [x1, y1] = [y1, x1]; }
    if (x0 > x1) { [x0, x1] = [x1, x0]; [y0, y1] = [y1, y0]; }
    const dx = x1 - x0, dy = Math.abs(y1 - y0), ystep = y0 < y1 ? 1 : -1;
    let err = (dx / 2) | 0;
    for (; x0 <= x1; x0++) {
      if (steep) px(g, y0, x0, c); else px(g, x0, y0, c);
      err -= dy;
      if (err < 0) { y0 += ystep; err += dx; }
    }
  }
  const inR = (dx, dy, r) => r >= 0 && dx * dx + dy * dy <= r * r + r;
  function circle(g, cx, cy, r, fill, c) {
    for (let dy = -r; dy <= r; dy++) for (let dx = -r; dx <= r; dx++)
      if (inR(dx, dy, r) && (fill || !inR(dx, dy, r - 1))) px(g, cx + dx, cy + dy, c);
  }
  // Is (i, j) inside a w x h rectangle with corner radius r? (i, j relative to its top-left)
  function inRR(i, j, w, h, r) {
    if (i < 0 || j < 0 || i >= w || j >= h) return false;
    const cx = i < r ? r : i > w - 1 - r ? w - 1 - r : i, cy = j < r ? r : j > h - 1 - r ? h - 1 - r : j;
    return inR(i - cx, j - cy, r);
  }
  function roundRect(g, x, y, w, h, r, fill, c) {
    if (w <= 0 || h <= 0) return;
    const m = (w < h ? w : h) >> 1;
    if (r > m) r = m;
    if (r < 0) r = 0;
    for (let j = 0; j < h; j++) for (let i = 0; i < w; i++)
      if (inRR(i, j, w, h, r) && (fill || !inRR(i - 1, j - 1, w - 2, h - 2, r - 1 < 0 ? 0 : r - 1))) px(g, x + i, y + j, c);
  }
  function bitmap(g, x, y, data, w, h, c, s) {
    const bw = (w + 7) >> 3;
    for (let j = 0; j < h; j++) for (let i = 0; i < w; i++)
      if (data[j * bw + (i >> 3)] & (0x80 >> (i & 7))) {
        if (s === 2) fillRect(g, x + i * 2, y + j * 2, 2, 2, c); else px(g, x + i, y + j, c);
      }
  }

  // ----------------------------------------------------------------- text
  // Text becomes a list of codes: printable ASCII, DEG for '°', '?' for anything else.
  function codes(s) {
    const out = [];
    for (const ch of s) {
      if (out.length >= 160) break;                 // OLED_MAXC on the watch
      const c = ch.codePointAt(0);
      out.push(c === 0xB0 ? DEG : c >= 32 && c <= 126 ? c : 63);
    }
    return out;
  }
  const glyph = (f, c) => f.glyphs.subarray((c - f.first) * 6, (c - f.first) * 6 + 6);   // offset, w, h, xAdvance, xOffset, yOffset
  const capOf = f => f.classic ? 7 : -glyph(f, 72)[5];
  const degR = f => { const cap = capOf(f); return cap >= 24 ? 3 : cap >= 16 ? 2 : 1; };
  function adv(f, c) {
    if (f.classic) return 6;
    if (c === DEG) return 2 * degR(f) + 3;
    return c >= f.first && c <= f.last ? glyph(f, c)[3] : 0;
  }
  function width(f, cs) { let w = 0; for (const c of cs) w += adv(f, c); return w; }
  function drawText(g, f, cs, x, y, col) {
    for (const c of cs) {
      if (f.classic) {
        const k = c === DEG ? 0xF8 : c;
        for (let i = 0; i < 5; i++) {
          let bits = f.bitmap[k * 5 + i];
          for (let j = 0; j < 8; j++, bits >>= 1) if (bits & 1) px(g, x + i, y - 7 + j, col);
        }
      } else if (c === DEG) {
        const r = degR(f);
        circle(g, x + r + 1, y - capOf(f) + r, r, false, col);
      } else if (c >= f.first && c <= f.last) {
        const [bo, w, h, , xo, yo] = glyph(f, c);
        let bit = 0, bits = 0, o = bo;
        for (let yy = 0; yy < h; yy++) for (let xx = 0; xx < w; xx++) {
          if (!(bit++ & 7)) bits = f.bitmap[o++];
          if (bits & 0x80) px(g, x + xo + xx, y + yo + yy, col);
          bits = (bits << 1) & 0xFF;
        }
      }
      x += adv(f, c);
    }
  }
  const DOTS = [46, 46];
  function ellipsize(f, cs, mw) {
    cs = cs.slice();
    while (cs.length && width(f, cs.concat(DOTS)) > mw) cs.pop();
    return cs.concat(DOTS);
  }
  const trimEnd = cs => { while (cs.length && cs[cs.length - 1] === 32) cs.pop(); return cs; };
  const trimStart = cs => { let i = 0; while (i < cs.length && cs[i] === 32) i++; return cs.slice(i); };
  // Splits into at most `lines` lines no wider than mw. fits = nothing was cut.
  function wrap(f, cs, mw, lines) {
    const out = [];
    let rest = trimStart(cs);
    while (rest.length && out.length < lines) {
      if (width(f, rest) <= mw) { out.push(rest); rest = []; break; }
      let n = 0;
      while (n < rest.length && width(f, rest.slice(0, n + 1)) <= mw) n++;
      if (n === 0) n = 1;
      let cut = n;
      if (rest[n] !== 32) { const sp = rest.lastIndexOf(32, n - 1); if (sp > 0) cut = sp; }
      out.push(trimEnd(rest.slice(0, cut)));
      rest = trimStart(rest.slice(cut));
    }
    const fits = rest.length === 0;
    if (!fits && out.length) out[out.length - 1] = ellipsize(f, out[out.length - 1], mw);
    return { out, fits };
  }

  // ---------------------------------------------------------------- fields
  // "{time} left" -> "10:25 left". Unknown fields stay as written, so a typo shows.
  function expand(s, field) {
    return str(s).replace(/\{([A-Za-z0-9_]+)\}/g, (m, k) => { const v = field(k); return v === undefined || v === null ? m : String(v); });
  }
  const upper = s => s.replace(/[a-z]/g, ch => ch.toUpperCase());   // ASCII only, as on the watch
  function shown(item, field) {
    const cond = str(item.if);
    if (!cond) return true;
    const neg = cond[0] === "!", v = field(neg ? cond.slice(1) : cond);
    const on = v !== undefined && v !== null && String(v) !== "" && String(v) !== "0";
    return neg ? !on : on;
  }

  // ---------------------------------------------------------------- items
  function drawItem(g, it, field) {
    const k = str(it.k), x = num(it.x, 0), y = num(it.y, 0), c = num(it.c, 1) ? 1 : 0;
    if (k === "text") {
      let s = expand(it.s, field);
      if (truthy(it.u)) s = upper(s);
      const cs = codes(s), list = Array.isArray(it.f) ? it.f : [it.f];
      const fonts = list.map(v => num(v, 0)).filter(v => v >= 0 && v < FONTS.length).slice(0, 8);
      if (!fonts.length) fonts.push(0);
      const mw = num(it.mw, g.w), lines = Math.min(8, Math.max(1, num(it.wrap, 1))), a = str(it.a);
      let f = null, res = null;
      for (const id of fonts) {
        f = FONTS[id];
        res = wrap(f, cs, mw, lines);
        if (res.fits) break;
      }
      const lh = num(it.lh, f.classic ? 9 : f.yAdvance);
      res.out.forEach((ln, i) => {
        const w = width(f, ln);
        const x0 = a === "c" ? x - ((w / 2) | 0) : a === "r" ? x - w : x;
        drawText(g, f, ln, x0, y + i * lh, c);
      });
    } else if (k === "icon") {
      const z = num(it.z, 16), data = ICONS[expand(it.n, field) + "@" + (z === 8 ? 8 : 16)];
      if (data) { const s = z === 8 ? 8 : 16; bitmap(g, x, y, data, s, s, c, z === 32 ? 2 : 1); }
    } else if (k === "battery") {
      const w = num(it.w, 20), h = num(it.h, 9);
      let p = parseInt(expand("{batt}", field), 10) || 0;
      p = p < 0 ? 0 : p > 100 ? 100 : p;
      const nub = (h / 3) | 0;
      rect(g, x, y, w - 2, h, c);
      fillRect(g, x + w - 2, y + nub, 2, h - 2 * nub, c);
      fillRect(g, x + 2, y + 2, (((w - 6) * p) / 100) | 0, h - 4, c);
    } else if (k === "bar") {
      const w = num(it.w, 40), h = num(it.h, 6), r = num(it.r, 0);
      let p = parseInt(expand(it.v, field), 10) || 0;
      p = p < 0 ? 0 : p > 100 ? 100 : p;
      if (h >= 5 && num(it.o, 1)) {
        roundRect(g, x, y, w, h, r, false, c);
        roundRect(g, x + 2, y + 2, (((w - 4) * p) / 100) | 0, h - 4, r - 2 < 0 ? 0 : r - 2, true, c);
      } else {
        roundRect(g, x, y, ((w * p) / 100) | 0, h, r, true, c);
      }
    } else if (k === "box") {
      roundRect(g, x, y, num(it.w, 10), num(it.h, 10), num(it.r, 0), truthy(it.fill), c);
    } else if (k === "line") {
      line(g, x, y, num(it.x2, x), num(it.y2, y), c);
    } else if (k === "circle") {
      circle(g, x, y, num(it.r, 4), truthy(it.fill), c);
    } else if (k === "bmp") {
      const w = num(it.w, 0), h = num(it.h, 0), data = b64(str(it.d));
      if (w > 0 && h > 0 && w <= 256 && h <= 256 && data.length >= ((w + 7) >> 3) * h) bitmap(g, x, y, data, w, h, c, 1);
    }
  }

  // design: a parsed screen file. field(name) -> string, or undefined if unknown.
  function render(design, field, w, h) {
    const g = canvas(w || num(design.w, 128), h || num(design.h, 32));
    for (const it of Array.isArray(design.items) ? design.items : [])
      if (it && typeof it === "object" && shown(it, field)) drawItem(g, it, field);
    if (truthy(design.inv)) for (let i = 0; i < g.px.length; i++) g.px[i] ^= 1;
    return g;
  }

  // Row-major, 8 pixels a byte, left pixel in the top bit: the "bmp" item format and the watch's oled_shot reply.
  function toBytes(g) {
    const bw = (g.w + 7) >> 3, out = new Uint8Array(bw * g.h);
    for (let y = 0; y < g.h; y++) for (let x = 0; x < g.w; x++) if (g.px[y * g.w + x]) out[y * bw + (x >> 3)] |= 0x80 >> (x & 7);
    return out;
  }
  function fromBytes(bytes, w, h) {
    const g = canvas(w, h), bw = (w + 7) >> 3;
    for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) g.px[y * w + x] = bytes[y * bw + (x >> 3)] & (0x80 >> (x & 7)) ? 1 : 0;
    return g;
  }
  const toB64 = bytes => { let s = ""; for (const b of bytes) s += String.fromCharCode(b); return btoa(s); };

  // Paints a canvas onto an HTML canvas element, looking like a lit OLED.
  function paint(el, g, opts = {}) {
    const s = opts.scale || 4, gap = s >= 4 ? 1 : 0;
    el.width = g.w * s; el.height = g.h * s;
    const ctx = el.getContext("2d");
    ctx.fillStyle = opts.bg || "#05070a";
    ctx.fillRect(0, 0, el.width, el.height);
    if (opts.grid) {
      ctx.fillStyle = "#11161d";
      for (let y = 0; y < g.h; y++) for (let x = 0; x < g.w; x++) ctx.fillRect(x * s, y * s, s - gap, s - gap);
    }
    ctx.fillStyle = opts.ink || "#9fe8ff";
    for (let y = 0; y < g.h; y++) for (let x = 0; x < g.w; x++) if (g.px[y * g.w + x]) ctx.fillRect(x * s, y * s, s - gap, s - gap);
  }

  // Image -> 1-bit bitmap (Floyd-Steinberg or a plain threshold). lum: Float32Array of 0..255.
  function dither(lum, w, h, threshold, mode, invert) {
    const L = Float32Array.from(lum), out = new Uint8Array(((w + 7) >> 3) * h), bw = (w + 7) >> 3;
    for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
      const i = y * w + x, old = L[i], on = old >= threshold, err = old - (on ? 255 : 0);
      if ((on ? 1 : 0) ^ (invert ? 1 : 0)) out[y * bw + (x >> 3)] |= 0x80 >> (x & 7);
      if (mode === "dither") {
        if (x + 1 < w) L[i + 1] += err * 7 / 16;
        if (y + 1 < h) {
          if (x > 0) L[i + w - 1] += err * 3 / 16;
          L[i + w] += err * 5 / 16;
          if (x + 1 < w) L[i + w + 1] += err / 16;
        }
      }
    }
    return out;
  }

  // Where an item sits, for picking it with the mouse in the editor.
  function bounds(it, field, g) {
    const k = str(it.k), x = num(it.x, 0), y = num(it.y, 0);
    if (k === "text") {
      const list = Array.isArray(it.f) ? it.f : [it.f];
      const f = FONTS[num(list[0], 0)] || FONTS[0];
      let s = expand(it.s, field); if (truthy(it.u)) s = upper(s);
      const mw = num(it.mw, g ? g.w : 128), w = Math.min(width(f, codes(s)), mw), a = str(it.a);
      const top = y - (f.classic ? 7 : capOf(f)), lines = Math.max(1, num(it.wrap, 1)), lh = num(it.lh, f.classic ? 9 : f.yAdvance);
      return { x: a === "c" ? x - ((w / 2) | 0) : a === "r" ? x - w : x, y: top, w: Math.max(w, 4), h: (f.classic ? 8 : capOf(f) + 2) + (lines - 1) * lh };
    }
    if (k === "icon") { const z = num(it.z, 16); return { x, y, w: z, h: z }; }
    if (k === "battery") return { x, y, w: num(it.w, 20), h: num(it.h, 9) };
    if (k === "bar") return { x, y, w: num(it.w, 40), h: num(it.h, 6) };
    if (k === "line") { const x2 = num(it.x2, x), y2 = num(it.y2, y); return { x: Math.min(x, x2), y: Math.min(y, y2), w: Math.abs(x2 - x) + 1, h: Math.abs(y2 - y) + 1 }; }
    if (k === "circle") { const r = num(it.r, 4); return { x: x - r, y: y - r, w: 2 * r + 1, h: 2 * r + 1 }; }
    return { x, y, w: num(it.w, 10), h: num(it.h, 10) };
  }

  return {
    FONTS: FONTS.map(f => f.name), ICON_NAMES: [...new Set(Object.keys(ICONS).map(k => k.split("@")[0]))].sort(),
    hasIcon: (name, size) => !!ICONS[name + "@" + size], BUILTIN: A.builtin,
    STARTERS: A.starters || {}, render, paint, toBytes, fromBytes, toB64, b64, dither, bounds, expand,
  };
})();
if (typeof module !== "undefined") module.exports = OLED;
