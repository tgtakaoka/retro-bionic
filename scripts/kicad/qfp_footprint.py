"""KiCad footprint of a gull-wing QFP, drawn from its package drawing.

Pads run from `heel` inside the foot's inner end to `toe` beyond the lead
tip, longer than IPC nominal so the leads can be hand soldered. Pin 1 is
at the top of the left row and the pins go counter-clockwise, as in
KiCad's own QFP footprints.
"""
import uuid

from sexp import format_text


def pair(value):
    return tuple(value) if isinstance(value, (tuple, list)) else (value, value)


def r3(v):
    return round(v * 1000) / 1000 + 0.0


def qfp_footprint(out, name, body, span, pitch, per_side, foot, pad_width=None, heel=0.55, toe=0.6,
                  descr='', model=None, model_rotate=0, pin1='corner', bumpers=None, silk_bevel=False):
    """body, span (lead tip to tip) and per_side are one value for a square package, or (x, y):
    per_side[0] pins on each left/right row, per_side[1] on each top/bottom row.
    foot: the flat foot length; pad_width defaults to 0.6 pitch, in 0.05 mm steps.
    model: a STEP file name in the project's bionic.pretty, rotated model_rotate degrees.
    pin1: 'corner' (top of the left row) or 'top-center' (the top row's middle pad, odd per_side[1]).
    bumpers: (width, outer, cut) of a square bumpered PQFP's diagonal corner arms, as qfp_step.py
    draws them; the courtyard wraps them and F.Fab outlines them.
    silk_bevel: cut the pin 1 corner of the silkscreen like the F.Fab outline, as on PLCCs."""
    body_x, body_y = pair(body)
    span_x, span_y = pair(span)
    count_x, count_y = pair(per_side)
    pad_w = pad_width or round(pitch * 0.6 / 0.05) * 0.05
    ns = uuid.uuid5(uuid.NAMESPACE_URL, name)
    n = [0]

    def uid():
        n[0] += 1
        return str(uuid.uuid5(ns, str(n[0])))

    def pad_geometry(span):
        inner = span / 2 - foot - heel
        outer = span / 2 + toe
        return (inner + outer) / 2, outer - inner, outer

    cx, len_x, outer_x = pad_geometry(span_x)
    cy, len_y, outer_y = pad_geometry(span_y)
    row_x = (count_x - 1) / 2 * pitch           # the left/right rows' half length
    row_y = (count_y - 1) / 2 * pitch
    pads = []
    num = 1
    for i in range(count_x):                    # left, top to bottom
        pads.append((num, -cx, -row_x + i * pitch, len_x, pad_w)); num += 1
    for i in range(count_y):                    # bottom, left to right
        pads.append((num, -row_y + i * pitch, cy, pad_w, len_y)); num += 1
    for i in range(count_x):                    # right, bottom to top
        pads.append((num, cx, row_x - i * pitch, len_x, pad_w)); num += 1
    for i in range(count_y):                    # top, right to left
        pads.append((num, row_y - i * pitch, -cy, pad_w, len_y)); num += 1
    if pin1 == 'top-center':
        assert count_y % 2, 'pin 1 at the top centre needs an odd top row'
        first = 2 * count_x + count_y + (count_y + 1) // 2
        total = len(pads)
        pads = [((k - first) % total + 1, x, y, w, h) for k, x, y, w, h in pads]

    def line(x0, y0, x1, y1, layer, width):
        return ('(fp_line (start %s %s) (end %s %s) (stroke (width %s) (type solid)) (layer "%s") (uuid "%s"))'
                % (r3(x0), r3(y0), r3(x1), r3(y1), width, layer, uid()))

    def prop(key, val, y, layer, hide=False):
        return ('(property "%s" "%s" (at 0 %s 0) (layer "%s")%s (uuid "%s") (effects (font (size 1 1) '
                '(thickness 0.15))))' % (key, val, r3(y), layer, ' (hide yes)' if hide else '', uid()))

    crt_x, crt_y = outer_x + 0.25, outer_y + 0.25               # courtyard: pads, then the body
    rows_x, rows_y = row_x + pad_w / 2 + 0.25, row_y + pad_w / 2 + 0.25
    body_cx, body_cy = body_x / 2 + 0.25, body_y / 2 + 0.25
    out_lines = ['(footprint "%s"' % name, '(version 20241229)', '(generator "pcbnew")',
                 '(generator_version "9.0")', '(layer "F.Cu")', '(descr "%s")' % descr, '(tags "QFP")',
                 prop('Reference', 'REF**', -(crt_y + 0.95), 'F.SilkS'),
                 prop('Value', name, crt_y + 0.95, 'F.Fab'),
                 prop('Footprint', '', 0, 'F.Fab', True),
                 prop('Datasheet', '', 0, 'F.Fab', True),
                 prop('Description', '', 0, 'F.Fab', True),
                 '(attr smd)']
    # silkscreen corners just outside the body, stopping 0.2 short of the end pads
    sx, sy = body_x / 2 + 0.11, body_y / 2 + 0.11
    ex, ey = row_y + pad_w / 2 + 0.2, row_x + pad_w / 2 + 0.2
    for fx in (-1, 1):
        for fy in (-1, 1):
            if silk_bevel and fx < 0 and fy < 0:
                # the pin 1 corner cut 1 mm like the F.Fab outline
                out_lines.append(line(-sx, -sy + 1, -sx, -ey, 'F.SilkS', 0.12))
                out_lines.append(line(-sx + 1, -sy, -ex, -sy, 'F.SilkS', 0.12))
                out_lines.append(line(-sx, -sy + 1, -sx + 1, -sy, 'F.SilkS', 0.12))
                continue
            out_lines.append(line(fx * sx, fy * sy, fx * sx, fy * ey, 'F.SilkS', 0.12))
            out_lines.append(line(fx * sx, fy * sy, fx * ex, fy * sy, 'F.SilkS', 0.12))
    if pin1 == 'top-center':
        # pin 1 triangle above pad 1, pointing down at it
        _, tx, py, _, ph = next(p for p in pads if p[0] == 1)
        tip = py - ph / 2 - 0.25
        tri = (tx, tip, tx - 0.34, tip - 0.47, tx + 0.34, tip - 0.47, tx, tip)
    else:
        # pin 1 triangle above the left row, pointing at pin 1
        tx = -(outer_x - 0.3)
        tip = -(row_x + pad_w / 2 + 0.28)
        tri = (tx, tip, tx - 0.34, tip - 0.47, tx + 0.34, tip - 0.47, tx, tip)
    out_lines.append('(fp_poly (pts (xy %s %s) (xy %s %s) (xy %s %s) (xy %s %s)) (stroke (width 0.12) (type solid)) '
                     '(fill yes) (layer "F.SilkS") (uuid "%s"))' % (tuple(r3(v) for v in tri) + (uid(),)))
    # fabrication outline with the pin 1 corner cut
    hx, hy = body_x / 2, body_y / 2
    for a, b in (((-hx + 1, -hy), (hx, -hy)), ((hx, -hy), (hx, hy)), ((hx, hy), (-hx, hy)),
                 ((-hx, hy), (-hx, -hy + 1)), ((-hx, -hy + 1), (-hx + 1, -hy))):
        out_lines.append(line(a[0], a[1], b[0], b[1], 'F.Fab', 0.1))
    if bumpers:
        # the arms' outlines, from the body edge round the cut tip
        width, outer, cut = bumpers
        k = width / 2 * 2 ** 0.5                # an arm edge's offset from the diagonal, along x
        arm = [(hx - k, hy), (outer - k, outer), (outer - cut, outer), (outer, outer - cut), (outer, outer - k),
               (hx, hy - k)]
        for fx, fy in ((1, -1), (1, 1), (-1, 1), (-1, -1)):
            for a, b in zip(arm, arm[1:]):
                out_lines.append(line(fx * a[0], fy * a[1], fx * b[0], fy * b[1], 'F.Fab', 0.1))
    # courtyard: a cross of the pad rows around the body square, or round the bumper arms
    if bumpers:
        kc, o = k + 0.25 * 2 ** 0.5, outer + 0.25
        tc = 2 * outer - cut + 0.25 * 2 ** 0.5
        corner = [(rows_y, rows_y + kc), (o - kc, o), (tc - o, o), (o, tc - o), (o, o - kc), (rows_x + kc, rows_x)]
    else:
        corner = [(rows_y, body_cy), (body_cx, body_cy), (body_cx, rows_x)]
    pts = []
    for fx, fy in ((1, -1), (1, 1), (-1, 1), (-1, -1)):
        q = [(rows_y, crt_y)] + corner + [(crt_x, rows_x)]
        if fx * fy > 0:
            q = q[::-1]
        pts += [(fx * x, fy * y) for x, y in q]
    pts = pts[-1:] + pts[:-1]
    for a, b in zip(pts, pts[1:] + pts[:1]):
        out_lines.append(line(a[0], a[1], b[0], b[1], 'F.CrtYd', 0.05))
    for num, x, y, w, h in pads:
        out_lines.append('(pad "%d" smd roundrect (at %s %s) (size %s %s) (layers "F.Cu" "F.Mask" "F.Paste") '
                         '(roundrect_rratio 0.25) (uuid "%s"))' % (num, r3(x), r3(y), r3(w), r3(h), uid()))
    out_lines.append('(embedded_fonts no)')
    if model:
        out_lines.append('(model "${KIPRJMOD}/bionic.pretty/%s" (offset (xyz 0 0 0)) (scale (xyz 1 1 1)) '
                         '(rotate (xyz 0 0 %s)))' % (model, model_rotate))
    out_lines.append(')')
    with open(out, 'w') as f:
        f.write(format_text('\n'.join(out_lines)))
