"""Autoroute the Watch V1 board. Pure Python, no dependencies.

    python route.py            reads ../kicad/watch_v1.pads.json (written by gen_pcb.py)
                               writes ../kicad/watch_v1.routes.json (read by gen_pcb.py)

A two-layer rip-up-and-reroute maze router on a 0.1 mm grid. Every connection
is an A* search from the already-routed part of its net to the next pad. For
each layer and each kind of new copper (signal track, power track, via) the
board remembers which nets' copper is too close to each grid cell, so a search
only has to look a cell up. All parts are through-hole, so every pad can be
reached on both layers.

When a connection is boxed in, the search is repeated with other nets' tracks
allowed at a high price; the nets it had to cross are ripped up and queued
again.
"""
import hashlib
import heapq
import json
import math
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KICAD_DIR = os.path.join(HERE, "..", "kicad")
PADS = os.path.join(KICAD_DIR, "watch_v1.pads.json")
OUT = os.path.join(KICAD_DIR, "watch_v1.routes.json")

G = 0.1                                   # grid pitch, mm
CLEARANCE = 0.2
MARGIN = 0.03                             # slack for grid sampling
W_SIGNAL, W_POWER = 0.3, 0.4
VIA_D, VIA_DRILL = 0.7, 0.35
EDGE_CLEARANCE = 0.4
HOLE_TO_HOLE = 0.25
SCREW_KEEPOUT_R = 2.4                     # M2 screw head / enclosure post
POWER_NETS = {"GND", "+3V3", "VBUS", "VBAT", "MOTOR_N"}

# Reach of foreign copper for each kind of new copper: signal, power, via.
REACH = [w / 2 + CLEARANCE + MARGIN for w in (W_SIGNAL, W_POWER, VIA_D)]
SIG, PWR, VIA = 0, 1, 2

# Soft keep-away (x0, y0, x1, y1, cost multiplier): the SuperMini's chip antenna.
SOFT = [(34.5, 10.0, 41.0, 21.5, 4.0)]

COST_VIA = 18.0
# Each layer has a preferred direction, which keeps tracks from fencing each
# other in: top (layer 0) runs mostly up-down, bottom (layer 1) left-right.
# Step cost by layer and direction index (see DIRS): E, SE, S, SW, W, NW, N, NE.
WRONG_WAY = 2.6
STEP = [[WRONG_WAY if dy == 0 else (1.0 if dx == 0 else 1.9) for dx, dy in
         ((1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1))],
        [WRONG_WAY if dx == 0 else (1.0 if dy == 0 else 1.9) for dx, dy in
         ((1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1))]]
COST_TURN = (0.0, 2.0, 6.0, 30.0, 60.0)   # by turn angle in 45 degree steps
COST_RIP = 25.0                           # per grid cell of another net's track; doubles each time that net is ripped
MAX_RIPS = 400
RIP_SECONDS = float(os.environ.get("ROUTE_SECONDS", 1500))   # after this, stop ripping and just finish
DIRS = [(1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1)]
BLOCKED = -1


class Board:
    def __init__(self, data):
        self.w, self.h = data["width"], data["height"]
        self.nx, self.ny = int(round(self.w / G)) + 1, int(round(self.h / G)) + 1
        n = self.nx * self.ny
        self.pad = [[[0] * n for _ in range(3)] for _ in range(2)]    # [layer][kind][cell] -> owning net (pads)
        self.trk = [[{} for _ in range(3)] for _ in range(2)]         # [layer][kind]{cell: {net: count}} (tracks, vias)
        self.no_via = bytearray(n)                                    # too close to a pad's hole
        self.via_near = {}                                            # {cell: count}: too close to a via's hole
        self.undo = {}                                                # net -> [(dict, cell)], for rip-up
        self.cost = [1.0] * n
        for x0, y0, x1, y1, mult in SOFT:
            for y in range(int(y0 / G), int(y1 / G) + 1):
                for x in range(int(x0 / G), int(x1 / G) + 1):
                    self.cost[y * self.nx + x] = mult
        edge = int(math.ceil((EDGE_CLEARANCE + W_POWER / 2) / G))
        self.lo, self.hi_x, self.hi_y = edge, self.nx - 1 - edge, self.ny - 1 - edge

    def _cells(self, dist, box, rmax):
        """(cell, distance) for every grid node within rmax of a copper shape."""
        x0, y0, x1, y1 = box
        for gy in range(max(0, int((y0 - rmax) / G)), min(self.ny - 1, int((y1 + rmax) / G) + 1) + 1):
            row = gy * self.nx
            for gx in range(max(0, int((x0 - rmax) / G)), min(self.nx - 1, int((x1 + rmax) / G) + 1) + 1):
                d = dist(gx * G, gy * G)
                if d <= rmax:
                    yield row + gx, d

    # -- fixed copper: pads and mounting holes
    def pad_rect(self, net, cx, cy, w, h):
        def dist(x, y):
            return math.hypot(max(abs(x - cx) - w / 2, 0), max(abs(y - cy) - h / 2, 0))
        for cell, d in self._cells(dist, (cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2), max(REACH)):
            for layer in (0, 1):
                for kind in range(3):
                    if d <= REACH[kind]:
                        m = self.pad[layer][kind]
                        m[cell] = net if m[cell] in (0, net) else BLOCKED

    def forbid_vias(self, cx, cy, r):
        for cell, _ in self._cells(lambda x, y: math.hypot(x - cx, y - cy), (cx, cy, cx, cy), r):
            self.no_via[cell] = 1

    # -- routed copper, which can be ripped up again
    def _add(self, net, layers, dist, box):
        undo = self.undo.setdefault(net, [])
        for cell, d in self._cells(dist, box, max(REACH)):
            for layer in layers:
                for kind in range(3):
                    if d <= REACH[kind]:
                        m = self.trk[layer][kind]
                        owners = m.get(cell)
                        if owners is None:
                            owners = m[cell] = {}
                        owners[net] = owners.get(net, 0) + 1
                        undo.append((m, cell))

    def add_segment(self, net, layer, ax, ay, bx, by, width):
        r = width / 2
        dx, dy = bx - ax, by - ay
        ll = dx * dx + dy * dy

        def dist(x, y):
            t = 0 if ll == 0 else max(0, min(1, ((x - ax) * dx + (y - ay) * dy) / ll))
            return math.hypot(x - ax - t * dx, y - ay - t * dy) - r
        self._add(net, [layer], dist, (min(ax, bx) - r, min(ay, by) - r, max(ax, bx) + r, max(ay, by) + r))

    def add_via(self, net, cx, cy):
        r = VIA_D / 2
        self._add(net, [0, 1], lambda x, y: math.hypot(x - cx, y - cy) - r, (cx - r, cy - r, cx + r, cy + r))
        undo = self.undo[net]
        for cell, _ in self._cells(lambda x, y: math.hypot(x - cx, y - cy), (cx, cy, cx, cy),
                                   VIA_DRILL + HOLE_TO_HOLE + MARGIN):
            self.via_near[cell] = self.via_near.get(cell, 0) + 1
            undo.append((None, cell))

    def rip(self, net):
        for m, cell in self.undo.pop(net, []):
            if m is None:
                self.via_near[cell] -= 1
                if not self.via_near[cell]:
                    del self.via_near[cell]
                continue
            owners = m[cell]
            owners[net] -= 1
            if not owners[net]:
                del owners[net]
                if not owners:
                    del m[cell]

    def node(self, x, y):
        return int(round(y / G)) * self.nx + int(round(x / G))

    def search(self, net, kind, sources, goal, rip_cost=None):
        """A* from any (layer, cell) in `sources` to the cell `goal` on either layer.

        rip_cost None: other nets' tracks are walls. Otherwise {net: price per
        cell} for running through them (the caller then rips those nets up).
        """
        nx = self.nx
        gx, gy = goal % nx, goal // nx
        pad = [self.pad[0][kind], self.pad[1][kind]]
        trk = [self.trk[0][kind], self.trk[1][kind]]
        pad_v0, pad_v1 = self.pad[0][VIA], self.pad[1][VIA]
        trk_v = [self.trk[0][VIA], self.trk[1][VIA]]
        cost, no_via, via_near = self.cost, self.no_via, self.via_near
        lo, hi_x, hi_y = self.lo, self.hi_x, self.hi_y
        best = {}
        heap = []
        for layer, cell in sources:
            best[(layer, cell)] = (0.0, None, -1)
            heap.append((self._h(cell, gx, gy), 0.0, layer, cell, -1))
        heapq.heapify(heap)
        while heap:
            _, g, layer, cell, came = heapq.heappop(heap)
            if best[(layer, cell)][0] < g:
                continue
            if cell == goal:
                path = []
                state = (layer, cell)
                while state is not None:
                    path.append(state)
                    state = best[state][1]
                return path[::-1]
            x, y = cell % nx, cell // nx
            layer_pad, layer_trk, step = pad[layer], trk[layer], STEP[layer]
            for d, (dx, dy) in enumerate(DIRS):
                x2, y2 = x + dx, y + dy
                if x2 < lo or y2 < lo or x2 > hi_x or y2 > hi_y:
                    continue
                c2 = y2 * nx + x2
                o = layer_pad[c2]
                if o != 0 and o != net:
                    continue
                extra = 0.0
                owners = layer_trk.get(c2)
                if owners is not None and (len(owners) > 1 or net not in owners):
                    if rip_cost is None:
                        continue
                    extra = sum(rip_cost[n] for n in owners if n != net)
                turn = 0 if came < 0 else min((d - came) % 8, (came - d) % 8)
                g2 = g + step[d] * cost[c2] + COST_TURN[turn] + extra
                s2 = (layer, c2)
                old = best.get(s2)
                if old is None or g2 < old[0]:
                    best[s2] = (g2, (layer, cell), d)
                    heapq.heappush(heap, (g2 + self._h(c2, gx, gy), g2, layer, c2, d))
            # change layer with a via
            if no_via[cell] or cell in via_near or pad_v0[cell] not in (0, net) or pad_v1[cell] not in (0, net):
                continue
            extra = 0.0
            for m in trk_v:
                owners = m.get(cell)
                if owners is not None and (len(owners) > 1 or net not in owners):
                    if rip_cost is None:
                        extra = -1.0
                        break
                    extra += 4 * sum(rip_cost[n] for n in owners if n != net)
            if extra < 0:
                continue
            s2 = (1 - layer, cell)
            g2 = g + COST_VIA + extra
            old = best.get(s2)
            if old is None or g2 < old[0]:
                best[s2] = (g2, (layer, cell), -1)
                heapq.heappush(heap, (g2 + self._h(cell, gx, gy), g2, 1 - layer, cell, -1))
        return None

    def _h(self, cell, gx, gy):
        dx, dy = abs(cell % self.nx - gx), abs(cell // self.nx - gy)
        return (max(dx, dy) + 0.4142 * min(dx, dy)) * 1.3

    def crossed(self, net, kind, path):
        """Other nets whose copper a rip-mode path runs into."""
        out = set()
        for i, (layer, cell) in enumerate(path):
            out.update(self.trk[layer][kind].get(cell, ()))
            if i + 1 < len(path) and path[i + 1][0] != layer:          # a via here
                out.update(self.trk[0][VIA].get(cell, ()))
                out.update(self.trk[1][VIA].get(cell, ()))
        out.discard(net)
        return out


def emit(board, tracks, name, net, run, width):
    """Merge collinear steps of one same-layer run into track segments."""
    if len(run) < 2:
        return
    layer = run[0][0]
    corners = [run[0]]
    for prev, cur, nxt in zip(run, run[1:], run[2:]):
        cross = (cur[1] - prev[1]) * (nxt[2] - cur[2]) - (cur[2] - prev[2]) * (nxt[1] - cur[1])
        if abs(cross) > 1e-9:
            corners.append(cur)
    corners.append(run[-1])
    for a, b in zip(corners, corners[1:]):
        if abs(a[1] - b[1]) < 1e-9 and abs(a[2] - b[2]) < 1e-9:
            continue
        tracks.append(dict(net=name, layer=layer, width=width,
                           x1=round(a[1], 4), y1=round(a[2], 4), x2=round(b[1], 4), y2=round(b[2], 4)))
        board.add_segment(net, layer, a[1], a[2], b[1], b[2], width)


def route(data):
    board = Board(data)
    nets, ids = {}, {}
    for i, p in enumerate(data["pads"]):
        if not p["plated"]:                               # mounting hole
            board.pad_rect(BLOCKED, p["x"], p["y"], 2 * SCREW_KEEPOUT_R, 2 * SCREW_KEEPOUT_R)
            board.forbid_vias(p["x"], p["y"], SCREW_KEEPOUT_R + 1)
            continue
        if p["net"]:
            ids.setdefault(p["net"], len(ids) + 1)
            nets.setdefault(p["net"], []).append(p)
        board.pad_rect(ids.get(p["net"]) or -(2 + i), p["x"], p["y"], p["w"], p["h"])   # unconnected pad: own owner
        board.forbid_vias(p["x"], p["y"], p["drill"] / 2 + HOLE_TO_HOLE + VIA_DRILL / 2 + MARGIN)
    names = {v: k for k, v in ids.items()}

    def span(name):
        ps = nets[name]
        return (name not in POWER_NETS,                 # power first: it has the most pads to reach
                max(p["x"] for p in ps) - min(p["x"] for p in ps) + max(p["y"] for p in ps) - min(p["y"] for p in ps))

    queue = [n for n in sorted(nets, key=span) if len(nets[n]) > 1]
    result = {}                                           # name -> (tracks, vias, unreached pads)
    rips = {ids[n]: 0 for n in nets}
    total_rips = 0
    started = time.time()
    while queue:
        name = queue.pop(0)
        pads = nets[name]
        net = ids[name]
        width, kind = (W_POWER, PWR) if name in POWER_NETS else (W_SIGNAL, SIG)
        tracks, vias, missed = [], [], []
        todo, done = pads[1:], [pads[0]]
        first = board.node(pads[0]["x"], pads[0]["y"])
        tree = {(0, first), (1, first)}
        while todo:
            nxt = min(todo, key=lambda p: min(math.hypot(p["x"] - q["x"], p["y"] - q["y"]) for q in done))
            todo.remove(nxt)
            done.append(nxt)
            goal = board.node(nxt["x"], nxt["y"])
            path = board.search(net, kind, tree, goal)
            if path is None and total_rips < MAX_RIPS and time.time() - started < RIP_SECONDS:
                price = {n: COST_RIP * 2 ** min(r, 6) * (8 if names[n] in POWER_NETS else 1) for n, r in rips.items()}
                path = board.search(net, kind, tree, goal, price)
                if path is not None:
                    for other in board.crossed(net, kind, path):
                        board.rip(other)
                        rips[other] += 1
                        total_rips += 1
                        result.pop(names[other], None)
                        if names[other] not in queue:
                            queue.append(names[other])
            if path is None:
                missed.append(f"{nxt['ref']}.{nxt['num']}")
                continue
            tree.update(path)
            tree.update({(0, goal), (1, goal)})
            nxg = board.nx
            pts = [(layer, (cell % nxg) * G, (cell // nxg) * G) for layer, cell in path]
            # Tie the ends to the exact pad centres (pads are not always on the grid).
            pts.append((pts[-1][0], nxt["x"], nxt["y"]))
            start = min(done[:-1], key=lambda q: math.hypot(q["x"] - pts[0][1], q["y"] - pts[0][2]))
            if math.hypot(start["x"] - pts[0][1], start["y"] - pts[0][2]) < G:
                pts.insert(0, (pts[0][0], start["x"], start["y"]))
            run = [pts[0]]
            for a, b in zip(pts, pts[1:]):
                if a[0] != b[0]:
                    emit(board, tracks, name, net, run, width)
                    vias.append(dict(net=name, x=round(a[1], 4), y=round(a[2], 4)))
                    board.add_via(net, a[1], a[2])
                    run = [b]
                else:
                    run.append(b)
            emit(board, tracks, name, net, run, width)
        result[name] = (tracks, vias, missed)
        print(f"  {name:12s} {len(tracks):3d} segments {len(vias)} vias" +
              (f"  UNREACHED {missed}" if missed else "") + f"   (queue {len(queue)}, rips {total_rips})", flush=True)
    tracks = [t for r in result.values() for t in r[0]]
    vias = [v for r in result.values() for v in r[1]]
    failed = [f"{n}: {m}" for n, r in result.items() for m in r[2]]
    return tracks, vias, failed


def main():
    with open(PADS, "rb") as f:
        raw = f.read()
    t = time.time()
    tracks, vias, failed = route(json.loads(raw))
    length = sum(math.hypot(s["x2"] - s["x1"], s["y2"] - s["y1"]) for s in tracks)
    print(f"{len(tracks)} segments, {len(vias)} vias, {length:.0f} mm of track, {time.time() - t:.0f} s")
    for item in failed:
        print("NOT ROUTED:", item)
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(dict(pads_sha1=hashlib.sha1(raw).hexdigest(), tracks=tracks, vias=vias,
                       via=dict(diameter=VIA_D, drill=VIA_DRILL)), f, indent=1)
    print("wrote watch_v1.routes.json")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
