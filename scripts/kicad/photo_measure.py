"""Measure a quad package from a top-down photo, for qfp_step.py and qfp_footprint.py.

photo_measure.py <photo> <body mm>[x<body mm>] [--rotate 90|180|270] [--ink <file.ppm>]

The body size from the package drawing sets the scale. The background is
the colour of the photo's border and the body the colour around its
centre; everything else outside the body is taken for leads. Prints, per
side, the lead count, pitch, width, length beyond the body and the end
leads' offsets from the corners, the lead span, and the corner cuts (the
pin 1 index is usually the largest). Measure the marking with --ink: it
writes the ink pixels as a PPM in mm-per-pixel scale with the body
outline and centre lines. The marking lines it lists (one per word where
words sit far apart) and the dimples read off directly as qfp_step.py's
lines and marks; font size is about cap height / 0.72.
Runs with KiCad's Python (wx reads the photo).
"""
import argparse
import sys

import wx


def load(path, rotate=0, max_side=2000):
    """(width, height, rgb bytes), turned rotate degrees counter-clockwise and scaled down to max_side"""
    img = wx.Image(path)
    if not img.IsOk():
        sys.exit('cannot read %s' % path)
    for _ in range(rotate // 90 % 4):
        img = img.Rotate90(False)
    w, h = img.GetWidth(), img.GetHeight()
    k = max(w, h) / max_side
    if k > 1:
        w, h = int(w / k), int(h / k)
        img = img.Scale(w, h, wx.IMAGE_QUALITY_HIGH)
    return w, h, bytes(img.GetData())


def median_colour(data, w, pts):
    chans = [sorted(data[(y * w + x) * 3 + c] for x, y in pts) for c in range(3)]
    return tuple(ch[len(ch) // 2] for ch in chans)


def classify(w, h, data):
    """per pixel: 0 background, 1 body, 2 other (leads, marking, glare).
    Shadows and edges blend the background and body colours, so a pixel
    is 'other' only when it is far from the line between those two."""
    border = [(x, y) for x in range(0, w, 4) for y in (0, 1, h - 2, h - 1)] + \
             [(x, y) for y in range(0, h, 4) for x in (0, 1, w - 2, w - 1)]
    bg = median_colour(data, w, border)
    cx, cy, r = w // 2, h // 2, min(w, h) // 10
    body = median_colour(data, w, [(x, y) for x in range(cx - r, cx + r, 3) for y in range(cy - r, cy + r, 3)])
    axis = [b - a for a, b in zip(bg, body)]
    sep = sum(c * c for c in axis)
    off_lim = sep / 9                           # a third of the colour distance, squared
    cls = bytearray(w * h)
    for i in range(w * h):
        d = (data[3 * i] - bg[0], data[3 * i + 1] - bg[1], data[3 * i + 2] - bg[2])
        t = (d[0] * axis[0] + d[1] * axis[1] + d[2] * axis[2]) / sep
        tc = min(1.0, max(0.0, t))
        off = (d[0] - tc * axis[0]) ** 2 + (d[1] - tc * axis[1]) ** 2 + (d[2] - tc * axis[2]) ** 2
        cls[i] = 2 if off > off_lim else 0 if t < 0.5 else 1
    return cls, bg, body


def runs(profile, threshold):
    """[(start, end)] where profile >= threshold"""
    out, start = [], None
    for i, v in enumerate(profile + [0]):
        if v >= threshold and start is None:
            start = i
        elif v < threshold and start is not None:
            out.append((start, i))
            start = None
    return out


def merged(spans, gap):
    """join runs closer than gap"""
    out = []
    for st, en in spans:
        if out and st - out[-1][1] < gap:
            out[-1] = (out[-1][0], en)
        else:
            out.append((st, en))
    return out


def fit_pitch(centres):
    n = len(centres)
    if n < 2:
        return 0
    mi = (n - 1) / 2
    mc = sum(centres) / n
    return sum((i - mi) * (c - mc) for i, c in enumerate(centres)) / sum((i - mi) ** 2 for i in range(n))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('photo')
    ap.add_argument('body', help='body size in mm, e.g. 14 or 14x20 (x, y as photographed)')
    ap.add_argument('--rotate', type=int, default=0, choices=(0, 90, 180, 270),
                    help='turn the photo counter-clockwise first, to bring pin 1 to the lower left')
    ap.add_argument('--ink', help='write the marking pixels to this PPM')
    a = ap.parse_args()
    _ = wx.App(False)
    bx_mm, by_mm = (float(v) for v in (a.body.split('x') + [a.body])[:2])
    w, h, data = load(a.photo, a.rotate)
    cls, bg, body = classify(w, h, data)
    print('photo %dx%d, background %s, body %s' % (w, h, bg, body))

    def at(x, y):
        return cls[y * w + x]

    rows = [sum(1 for x in range(w) if cls[y * w + x] == 1) for y in range(h)]
    cols = [sum(1 for y in range(h) if cls[y * w + x] == 1) for x in range(w)]
    y0, y1 = (lambda r: (r[0], r[-1]))([y for y, n in enumerate(rows) if n > max(rows) / 2])
    x0, x1 = (lambda r: (r[0], r[-1]))([x for x, n in enumerate(cols) if n > max(cols) / 2])
    sx, sy = (x1 - x0) / bx_mm, (y1 - y0) / by_mm
    s = (sx + sy) / 2
    print('body x %d-%d, y %d-%d: %.1f x %.1f px/mm (aspect %.3f)' % (x0, x1, y0, y1, sx, sy, sx / sy))

    def mm(px):
        return px / s

    # each side: the lead band outside the body edge, scanned outwards
    sides = {
        'top':    (lambda d, t: (t, y0 - 1 - d), x0, x1, y0),
        'bottom': (lambda d, t: (t, y1 + 1 + d), x0, x1, h - 1 - y1),
        'left':   (lambda d, t: (x0 - 1 - d, t), y0, y1, x0),
        'right':  (lambda d, t: (x1 + 1 + d, t), y0, y1, w - 1 - x1),
    }
    spans = {}
    for name, (xy, t0, t1, room) in sides.items():
        along = range(t0, t1 + 1)
        counts = [sum(1 for t in along if at(*xy(d, t)) == 2) for d in range(room)]
        noise = (t1 - t0) * 0.02
        # leads run unbroken from the body edge; specks further out are not leads
        peak = max(range(room), key=lambda d: counts[d]) if room else 0
        if not room or counts[peak] <= noise:
            print('%-6s no leads' % name)
            continue
        reach = next((d for d in range(peak, room) if counts[d] <= noise), room)
        if reach == 0:
            print('%-6s no leads' % name)
            continue
        # lead profile across the middle half of the lead length
        band = range(reach // 4, max(reach // 4 + 1, reach * 3 // 4))
        prof = [sum(1 for d in band if at(*xy(d, t)) == 2) / len(band) for t in along]
        leads = runs(prof, 0.5)
        if not leads:
            print('%-6s no leads' % name)
            continue
        # glare can split a lead: join pieces closer than a quarter of the typical spacing
        gaps = sorted(b[0] - a[1] for a, b in zip(leads, leads[1:]))
        joined = [list(leads[0])]
        min_gap = gaps[len(gaps) // 2] / 4 if gaps else 0
        for st, en in leads[1:]:
            if st - joined[-1][1] < min_gap:
                joined[-1][1] = en
            else:
                joined.append([st, en])
        centres = [(st + en) / 2 for st, en in joined]
        widths = sorted(en - st for st, en in joined)
        pitch = fit_pitch(centres)
        spans[name] = reach
        print('%-6s %2d leads, pitch %.3f, width %.3f, length %.2f, end leads %.2f / %.2f from the corners'
              % (name, len(joined), mm(pitch), mm(widths[len(widths) // 2]), mm(reach),
                 mm(centres[0]), mm(t1 - t0 - centres[-1])))
    if 'left' in spans and 'right' in spans:
        print('span x %.2f' % mm(x1 - x0 + spans['left'] + spans['right']))
    if 'top' in spans and 'bottom' in spans:
        print('span y %.2f' % mm(y1 - y0 + spans['top'] + spans['bottom']))

    # corner cuts: the first body pixel on the diagonal from each bbox corner
    for name, (cx, cy, dx, dy) in (('top-left', (x0, y0, 1, 1)), ('top-right', (x1, y0, -1, 1)),
                                   ('bottom-left', (x0, y1, 1, -1)), ('bottom-right', (x1, y1, -1, -1))):
        d = next((d for d in range(min(x1 - x0, y1 - y0) // 2) if at(cx + dx * d, cy + dy * d) == 1), 0)
        print('corner %-12s cut %.2f (legs)' % (name, mm(2 * d)))

    # marking: connected ink pieces inside the body. Round filled ones are mold dimples; pieces
    # at least 0.5 mm tall are glyphs, grouped into lines; the rest is glare and dust.
    m = int(min(x1 - x0, y1 - y0) * 0.03)
    ix0, ix1, iy0, iy1 = x0 + m, x1 - m, y0 + m, y1 - m

    def ink(x, y):
        return at(x, y) == 2

    seen = bytearray(w * h)
    pieces = []
    for y in range(iy0, iy1):
        for x in range(ix0, ix1):
            if seen[y * w + x] or not ink(x, y):
                continue
            stack, n = [(x, y)], 0
            seen[y * w + x] = 1
            px0 = px1 = x
            py0 = py1 = y
            while stack:
                cx, cy = stack.pop()
                n += 1
                px0, px1, py0, py1 = min(px0, cx), max(px1, cx), min(py0, cy), max(py1, cy)
                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if ix0 <= nx < ix1 and iy0 <= ny < iy1 and not seen[ny * w + nx] and ink(nx, ny):
                        seen[ny * w + nx] = 1
                        stack.append((nx, ny))
            pieces.append((px0, px1 + 1, py0, py1 + 1, n))
    mx, my = (x0 + x1) / 2, (y0 + y1) / 2

    def uv(px, py):
        return mm(px - mx), mm(my - py)

    dimples, glyphs = [], []
    for px0, px1, py0, py1, n in pieces:
        pw, ph = px1 - px0, py1 - py0
        if mm(min(pw, ph)) > 0.6 and 0.8 < pw / ph < 1.25 and n > 0.65 * 3.1416 * pw * ph / 4:
            dimples.append((px0, px1, py0, py1))
        elif mm(ph) >= 0.5:
            glyphs.append((px0, px1, py0, py1))
    for px0, px1, py0, py1 in dimples:
        print('dimple   u %6.2f  v %6.2f  radius %.2f' % (uv((px0 + px1) / 2, (py0 + py1) / 2)
                                                         + (mm((px1 - px0 + py1 - py0) / 4),)))
    # a glyph joins a line it overlaps vertically by half its height and sits within a glyph height of
    lines = []
    for g in sorted(glyphs):
        gh = g[3] - g[2]
        for ln in lines:
            ov = min(ln[3], g[3]) - max(ln[2], g[2])
            if ov > gh / 2 and g[0] - ln[1] < gh:
                ln[1], ln[2], ln[3] = max(ln[1], g[1]), min(ln[2], g[2]), max(ln[3], g[3])
                ln[4] += 1
                break
        else:
            lines.append([g[0], g[1], g[2], g[3], 1])
    print('marking lines, pin 1 at the lower left as photographed (u, v mm from the body centre):')
    for lx0, lx1, ly0, ly1, n in sorted(lines, key=lambda ln: (ln[2], ln[0])):
        u0, v = uv(lx0, (ly0 + ly1) / 2)
        print('  %2d glyphs  u from %6.2f  width %5.2f  v %6.2f  cap height %.2f'
              % (n, u0, mm(lx1 - lx0), v, mm(ly1 - ly0)))

    if a.ink:
        # marking pixels in black, the body outline and centre lines in grey
        with open(a.ink, 'wb') as f:
            f.write(b'P6\n%d %d\n255\n' % (x1 - x0 + 1, y1 - y0 + 1))
            cx, cy = (x0 + x1) // 2, (y0 + y1) // 2
            for y in range(y0, y1 + 1):
                line = bytearray()
                for x in range(x0, x1 + 1):
                    v = 0 if ink(x, y) else 128 if x in (x0, x1, cx) or y in (y0, y1, cy) else 255
                    line += bytes((v, v, v))
                f.write(line)
        print('ink image: %s' % a.ink)

if __name__ == '__main__':
    main()
