"""Draws the pictures for the themed clock faces and writes tools/oled/art.txt.

Each picture is made of simple shapes (ellipses, polygons, lines) laid out on a
32 x 32 grid, then drawn at the size each panel needs: 32 px for 128x32 panels,
48 px for 128x64. A pixel is lit when its centre falls inside a shape, which
gives crisp 1-bit edges at any size.

    python tools/oled/draw_art.py        rewrites art.txt
    python tools/oled/draw_art.py --show prints every picture to the terminal

Some pictures have a second frame: "~blink" (eyes shut) or "~flap" (wings
down). The faces swap to it with the {blink} / {tick} fields, so the animals
blink now and then and the bee's wings flap once a second.

art.txt can also be edited by hand; this script overwrites it, so hand-drawn
pictures belong in art_hand.txt, which build_assets.py reads as well.
"""
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SIZES = (32, 48)


class Pic:
    def __init__(self, n):
        self.n, self.k = n, n / 32.0
        self.px = [[0] * n for _ in range(n)]

    def _paint(self, inside, on):
        for y in range(self.n):
            for x in range(self.n):
                if inside((x + 0.5) / self.k, (y + 0.5) / self.k):
                    self.px[y][x] = 1 if on else 0

    # All coordinates and sizes are in the 32-unit grid.
    def ellipse(self, cx, cy, rx, ry, on=True, rot=0.0, ring=0.0):
        c, s = math.cos(math.radians(rot)), math.sin(math.radians(rot))
        t = ring / self.k if ring else 0     # ring thickness is in pixels, so lines stay 1-2 px at any size
        t = max(t, 0)

        def inside(x, y, rx=rx, ry=ry):
            dx, dy = x - cx, y - cy
            u, v = dx * c + dy * s, -dx * s + dy * c
            return (u / rx) ** 2 + (v / ry) ** 2 <= 1
        if ring:
            self._paint(lambda x, y: inside(x, y) and not inside(x, y, rx - t, ry - t), on)
        else:
            self._paint(inside, on)

    def circle(self, cx, cy, r, on=True, ring=0.0):
        self.ellipse(cx, cy, r, r, on, 0, ring)

    def poly(self, pts, on=True):
        def inside(x, y):
            hit = False
            for i in range(len(pts)):
                (x1, y1), (x2, y2) = pts[i], pts[i - 1]
                if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
                    hit = not hit
            return hit
        self._paint(inside, on)

    def line(self, x1, y1, x2, y2, w=1.0, on=True):
        w = w / self.k / 2 + 0.01

        def inside(x, y):
            dx, dy = x2 - x1, y2 - y1
            L = dx * dx + dy * dy or 1
            t = max(0, min(1, ((x - x1) * dx + (y - y1) * dy) / L))
            return math.hypot(x - x1 - t * dx, y - y1 - t * dy) <= w
        self._paint(inside, on)

    def rect(self, x1, y1, x2, y2, on=True):
        self._paint(lambda x, y: x1 <= x < x2 and y1 <= y < y2, on)

    def text(self):
        return ["".join("#" if p else "." for p in row) for row in self.px]


# ---------------------------------------------------------------- the pictures
# Each takes (pic, frame); frame is "" or the name of the second frame.

def bee(p, f):
    flap = f == "flap"
    for cx, rot in ((13, -25), (20, 20)):           # wings, outlined
        p.ellipse(cx, 11 if flap else 8, 4.5, 7, rot=rot * (1.8 if flap else 1), ring=1.3)
    p.ellipse(18, 20, 12, 9, on=False)              # gap between wings and body
    p.ellipse(18, 20, 11, 8)                        # body
    for x in (14, 19.5):                            # stripes
        p.rect(x, 12, x + 2.6, 29, on=False)
    p.ellipse(18, 20, 11, 8, ring=1.3)              # keeps the outline round the stripes
    p.poly([(28.5, 18.6), (31.8, 20), (28.5, 21.4)])  # sting
    p.circle(7, 19, 6.5, on=False)
    p.circle(7, 19, 5.5)                            # head
    p.circle(5.2, 18, 1.3, on=False)                # eye
    p.line(3.5, 21.6, 6, 22.6, on=False)            # smile
    p.line(6, 14, 3, 7)                             # antennae
    p.line(9, 14, 10, 7)
    p.circle(3, 7, 1.2)
    p.circle(10, 7, 1.2)


def fox(p, f):
    p.poly([(3, 3), (12, 11), (20, 11), (29, 3), (28, 17), (16, 30), (4, 17)])
    for s in (1, -1):                               # inner ears
        x = lambda v: v if s == 1 else 32 - v
        p.poly([(x(5.5), 7), (x(10.5), 11.6), (x(6.5), 13.5)], on=False)
    # pale cheeks: an outline of the lower face
    p.line(5, 18, 13, 23, on=False, w=1.2)
    p.line(27, 18, 19, 23, on=False, w=1.2)
    if f == "blink":
        p.line(9, 17, 13, 17, on=False, w=1.3)
        p.line(19, 17, 23, 17, on=False, w=1.3)
    else:
        p.ellipse(11, 16.5, 1.6, 2.2, on=False)
        p.ellipse(21, 16.5, 1.6, 2.2, on=False)
    p.ellipse(16, 25.2, 2.4, 1.7, on=False)         # nose


def cat(p, f):
    for s in (1, -1):
        x = lambda v: v if s == 1 else 32 - v
        p.poly([(x(3.5), 15), (x(6), 2.5), (x(14), 10)])          # ears
        p.poly([(x(6), 12), (x(7.3), 6.5), (x(11), 10.5)], on=False)
    p.ellipse(16, 19, 13, 10.5, on=False)
    p.ellipse(16, 19, 12, 9.5, ring=1.4)            # head
    if f == "blink":
        p.line(9.5, 17.5, 12.5, 17.5, w=1.3)
        p.line(19.5, 17.5, 22.5, 17.5, w=1.3)
    else:
        p.ellipse(11, 17, 1.6, 2.4)
        p.ellipse(21, 17, 1.6, 2.4)
    p.poly([(14.6, 21), (17.4, 21), (16, 22.6)])    # nose
    p.line(16, 22.6, 14, 24.5)
    p.line(16, 22.6, 18, 24.5)
    for s in (1, -1):                               # whiskers
        x = lambda v: v if s == 1 else 32 - v
        p.line(x(8), 21, x(0.5), 19.5)
        p.line(x(8), 23, x(0.5), 24.5)


def dog(p, f):
    p.ellipse(16, 17, 10, 11, ring=1.4)             # head
    for s in (1, -1):                               # floppy ears, filled
        cx = 16 - 11.5 * s
        p.ellipse(cx, 16, 4.4, 8.5, on=False, rot=18 * s)
        p.ellipse(cx, 16, 3.3, 7.5, rot=18 * s)
    p.ellipse(16, 23.5, 5.5, 4.5, ring=1.3)         # muzzle
    p.ellipse(16, 21, 2.6, 1.8)                     # nose
    p.line(16, 22.5, 16, 25)
    if f == "blink":
        p.line(10.5, 15, 13.5, 15, w=1.3)
        p.line(18.5, 15, 21.5, 15, w=1.3)
    else:
        p.circle(12, 15, 1.7)
        p.circle(20, 15, 1.7)
    p.ellipse(16, 28.6, 1.8, 1.6)                   # tongue


def owl(p, f):
    p.ellipse(16, 18.5, 12, 13)                     # body
    for s in (1, -1):
        x = lambda v: v if s == 1 else 32 - v
        p.poly([(x(4.5), 9), (x(6), 1.5), (x(12), 7)])            # ear tufts
    for cx in (10.5, 21.5):
        p.circle(cx, 13.5, 5.2, on=False)           # eye discs
        if f == "blink":
            p.line(cx - 3, 13.5, cx + 3, 13.5, w=1.4)
        else:
            p.circle(cx, 13.5, 2.4)
    p.poly([(14, 18), (18, 18), (16, 23)], on=False)              # beak
    for x, y in ((12.5, 25.5), (19.5, 25.5), (16, 28.5)):
        p.line(x - 1.5, y - 1, x, y, on=False)      # feathers: little v's
        p.line(x, y, x + 1.5, y - 1, on=False)


def rabbit(p, f):
    for s in (1, -1):                               # ears
        cx = 16 - 4.5 * s
        p.ellipse(cx, 9, 3.4, 8.5, rot=-12 * s, ring=1.3)
        p.line(cx + 0.3 * s, 4, cx + 1.2 * s, 13)
    p.ellipse(16, 22, 10.5, 8.5, on=False)
    p.ellipse(16, 22, 9.5, 7.5, ring=1.4)           # head
    if f == "blink":
        p.line(10.5, 20.5, 13.5, 20.5, w=1.3)
        p.line(18.5, 20.5, 21.5, 20.5, w=1.3)
    else:
        p.circle(12, 20.5, 1.5)
        p.circle(20, 20.5, 1.5)
    p.poly([(14.8, 23.5), (17.2, 23.5), (16, 25)])
    p.line(16, 25, 16, 26.5)
    for s in (1, -1):
        x = lambda v: v if s == 1 else 32 - v
        p.line(x(9.5), 25, x(2.5), 24)
        p.line(x(9.5), 26.5, x(3), 28)


def graph(p, f):                                    # maths: axes and a sine curve
    p.line(3, 16, 30, 16)
    p.line(6, 3, 6, 29)
    p.poly([(30.5, 16), (27.5, 14.3), (27.5, 17.7)])
    p.poly([(6, 2), (4.3, 5), (7.7, 5)])
    pts = [(6 + i * 0.25, 16 - 9 * math.sin(i * 0.25 / 22 * 2 * math.pi)) for i in range(89)]
    for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
        p.line(x1, y1, x2, y2, w=1.6)
    p.circle(11.5, 16 - 9, 1.8)                     # a point on the curve


def atom(p, f):
    for rot in (0, 60, 120):
        p.ellipse(16, 16, 14, 5.2, rot=rot, ring=1.2)
    p.circle(16, 16, 3.2, on=False)
    p.circle(16, 16, 2.4)
    p.circle(30, 16, 1.6)                           # an electron


def music(p, f):                                    # two beamed quavers
    p.poly([(10, 6), (27, 3), (27, 7), (10, 10)])
    p.rect(9.4, 7, 11, 25)
    p.rect(25.4, 4, 27, 22)
    p.ellipse(7, 25.5, 4, 3, rot=-20)
    p.ellipse(23, 22.5, 4, 3, rot=-20)


def book(p, f):                                     # an open book
    for s in (1, -1):
        x = lambda v: v if s == 1 else 32 - v
        p.poly([(x(16), 9), (x(4), 5), (x(2), 25), (x(16), 28)])
        p.poly([(x(15), 11), (x(5.5), 7.6), (x(4), 23), (x(15), 25.6)], on=False)
        for y in (11.5, 15, 18.5):
            p.line(x(7), y - 0.8 + (0 if s == 1 else 0), x(13), y + 1.4)
    p.line(16, 9, 16, 28)


def sprout(p, f):                                   # a seedling in a pot: biology / growth
    p.poly([(8, 21), (24, 21), (22, 30), (10, 30)])
    p.rect(7, 19, 25, 22)
    p.line(16, 19, 16, 9, w=1.4)
    p.ellipse(10.5, 10, 6, 3, rot=-30)
    p.ellipse(21.5, 7, 6, 3, rot=30)
    p.line(10.5, 10, 16, 13, on=False)
    p.line(21.5, 7, 16, 10.5, on=False)


PICTURES = [
    ("bee", bee, "flap"), ("fox", fox, "blink"), ("cat", cat, "blink"), ("dog", dog, "blink"),
    ("owl", owl, "blink"), ("rabbit", rabbit, "blink"),
    ("graph", graph, ""), ("atom", atom, ""), ("music", music, ""), ("book", book, ""), ("sprout", sprout, ""),
]


def main():
    out = ["# Pictures for the themed clock faces. Written by tools/oled/draw_art.py: do not edit by hand",
           "# (put hand-drawn pictures in art_hand.txt instead). '#' = lit, '.' = dark.",
           "# Each starts with '@name width height'. build_assets.py turns them into the faces' bitmaps.", ""]
    for name, fn, extra in PICTURES:
        for n in SIZES:
            for frame in [""] + ([extra] if extra else []):
                p = Pic(n)
                fn(p, frame)
                out.append("@%s%s %d %d" % (name, "~" + frame if frame else "", n, n))
                out += p.text()
    open(os.path.join(HERE, "art.txt"), "w", newline="\n").write("\n".join(out) + "\n")
    if "--show" in sys.argv:
        print("\n".join(out))
    print("art.txt: %d pictures" % sum(1 for l in out if l.startswith("@")))


if __name__ == "__main__":
    main()
