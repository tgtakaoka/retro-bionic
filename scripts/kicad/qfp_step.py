"""STEP model of a gull-wing QFP with its top marking, for a footprint's 3D view.

Needs build123d (not in KiCad's Python): run with the CAD venv, e.g.
~/.venvs/cad/bin/python. Dimensions come from the package drawing; the
marking lines and mold marks from a photo of the part.
"""
import math

from build123d import (Align, Axis, Box, Circle, Color, Compound, Cylinder, FontStyle, Plane, Polygon, Pos, Rectangle,
                       Rot, Text, chamfer, export_step, extrude)

PIN1_DEPTH = 0.3    # the pin 1 dimple's depth
BEND = 0.2          # a lead's bend radius, at its centre line


def offset(line, d):
    """the polyline |line| moved |d| to its left, corners mitred"""
    def normal(p, q):
        dx, dy = q[0] - p[0], q[1] - p[1]
        n = math.hypot(dx, dy)
        return -dy / n, dx / n
    segs = list(zip(line, line[1:]))
    normals = [normal(p, q) for p, q in segs]
    out = [(line[0][0] + d * normals[0][0], line[0][1] + d * normals[0][1])]
    for (_, q), n0, n1 in zip(segs, normals, normals[1:]):
        mx, my = n0[0] + n1[0], n0[1] + n1[1]
        k = d / (1 + n0[0] * n1[0] + n0[1] * n1[1])
        out.append((q[0] + k * mx, q[1] + k * my))
    out.append((line[-1][0] + d * normals[-1][0], line[-1][1] + d * normals[-1][1]))
    return out


def pair(value):
    return tuple(value) if isinstance(value, (tuple, list)) else (value, value)


def qfp(out, label, body, span, pitch, per_side, lead_width, lead_thick, foot, standoff, thick,
        lines=(), logo=None, pin1=None, marks=(), dots=(), rects=(), circled=(), font=None, index=None,
        bumpers=None, mark_rot=-90, bevel=None):
    """body, span (lead tip to tip) and per_side (leads on each side) are one value for a square
    package, or (x, y) in the footprint's frame for a rectangular one: per_side[0] leads on each
    X side, the side pin 1 starts.
    Marking positions (u, v) are mm from the body centre as read with pin 1 at the lower left:
    lines: [(text, v, font_size)] centred, or [(text, v, font_size, u, width)] from u at the
    left, its glyphs spread evenly over width
    logo: (letter, v) or (letter, v, ring radius) in a ring, ('freescale', v, width) for its tiles, or
    ('motorola', u, v, ring radius) for its batwing
    pin1: (u, v, radius) the pin 1 dimple, PIN1_DEPTH deep; marks: [(u, v, radius, depth)] other round
    dimples, e.g. ejector pin marks; dots: [(u, v, radius)] printed dots; rects: [(u, v, width,
    height)] printed rectangles, centred; circled: [(letter, u, v, ring radius)], e.g. (M) and (C)
    font: a font file for the text, else build123d's own
    index: the pin 1 corner's 45 degree cut, its legs this long, through the body's full height
    bumpers: (width, outer, cut) or (width, outer, cut, floor) corner bumpers of a bumpered PQFP:
    arms width wide running diagonally out of the body corners to outer mm off the centre on
    both axes, their tips cut cut mm in from the corner, from floor mm above the seating plane
    (default: the body's underside) to the body top
    mark_rot: the marking's turn, -90 for pin 1 at the lower left as read, 0 for the upper left
    bevel: (side, size) a 45 degree bevel along the body's top edge on side '-x', '+x', '-y' or '+y'
    (pin 1 is the top lead of the -X side), its legs size long, e.g. Intel's pin 1 side"""
    body_x, body_y = pair(body)
    span_x, span_y = pair(span)
    count_x, count_y = pair(per_side)
    zmid = standoff + thick / 2
    top = standoff + thick
    b = Box(body_x, body_y, thick, align=(Align.CENTER, Align.CENTER, Align.MIN))
    b = Pos(0, 0, standoff) * chamfer(b.edges().group_by(Axis.Z)[-1], 0.2)
    if bevel:
        side, size = bevel
        sign = 1 if side[0] == '+' else -1
        s = size * 2 ** 0.5
        if side[1] == 'x':
            b = b - Pos(sign * body_x / 2, 0, top) * Rot(0, 45, 0) * Box(s, body_y + 1, s)
        else:
            b = b - Pos(0, sign * body_y / 2, top) * Rot(45, 0, 0) * Box(body_x + 1, s, s)
    if bumpers:
        width, outer, cut, floor = (tuple(bumpers) + (standoff,))[:4]
        h = top - floor
        bound = Pos(0, 0, floor) * Box(2 * outer, 2 * outer, h, align=(Align.CENTER, Align.CENTER, Align.MIN))
        for sx in (-1, 1):
            for sy in (-1, 1):
                d = outer / 2
                arm = Pos(sx * d, sy * d, floor) * Rot(0, 0, 45 * sx * sy) * \
                    Box(outer * 2 ** 0.5, width, h, align=(Align.CENTER, Align.CENTER, Align.MIN))
                tip = Pos(sx * outer, sy * outer, 0) * Rot(0, 0, 45) * Box(cut * 2 ** 0.5, cut * 2 ** 0.5, 20)
                b = b + ((arm & bound) - tip)
    if index:
        # pin 1 is the top lead of the -X side
        b = b - Pos(-body_x / 2, body_y / 2, 0) * Rot(0, 0, 45) * Box(index * 2 ** 0.5, index * 2 ** 0.5, 10)
    for u, v, r, depth in ([pin1 + (PIN1_DEPTH,)] if pin1 else []) + list(marks):
        b = b - Rot(0, 0, mark_rot) * (Pos(u, v, top - depth) *
                                      Cylinder(r, depth + 0.05, align=(Align.CENTER, Align.CENTER, Align.MIN)))

    def gull_wing(body, span):
        """A lead pointing to -X, centred on y = 0."""
        xe = -body / 2
        xt = -span / 2
        xk = xt + foot                 # heel
        # the lead's centre line, body to tip, drawn as a strip lead_thick thick: out of the body
        # level, down at the heel through two bends of BEND, and the foot flat (Intel's Detail A)
        xv, zf = xk + BEND, lead_thick / 2
        arc = [math.radians(a) for a in range(0, 91, 15)]
        line = ([(xe + 0.1, zmid)] +
                [(xv + BEND - BEND * math.sin(a), zmid - BEND + BEND * math.cos(a)) for a in arc] +
                [(xv - BEND + BEND * math.cos(a), zf + BEND - BEND * math.sin(a)) for a in arc] +
                [(xt, zf)])
        side = [offset(line, d) for d in (lead_thick / 2, -lead_thick / 2)]
        profile = Plane.XZ * Polygon(*(side[0] + side[1][::-1]), align=None)
        return extrude(profile, lead_width / 2, both=True)

    # sides -X, -Y, +X, +Y
    sides = [(gull_wing(body_x, span_x), count_x), (gull_wing(body_y, span_y), count_y)]
    leads = Compound([Rot(0, 0, 90 * side) * (Pos(0, (n - 1) / 2 * pitch - i * pitch, 0) * lead)
                      for side in range(4) for lead, n in [sides[side % 2]] for i in range(n)])

    def ink(shape, u, v):
        return Rot(0, 0, mark_rot) * (Pos(u, v, top) * extrude(shape, 0.01))

    def text(t, size, style=FontStyle.REGULAR):
        return Text(t, font_size=size, font_path=font, font_style=style)

    def ringed(ch, u, v, r):
        return [ink(Circle(r) - Circle(r * 0.87), u, v), ink(text(ch, r * 1.1, FontStyle.BOLD), u, v)]

    ink_parts = []
    for t, v, size, *left in lines:
        if left:
            u, width = left
            cell = width / len(t)
            ink_parts += [ink(text(ch, size), u + (i + 0.5) * cell, v) for i, ch in enumerate(t) if ch != ' ']
        else:
            ink_parts.append(ink(text(t, size), 0, v))
    if logo and logo[0] == 'freescale':
        # Freescale's mark: slanted tiles stacked in a diamond, size wide
        _, v, size = logo
        s = size / 4
        for x, y in ((0, 1.5), (-0.5, 0.5), (0.5, 0.5), (-1, -0.5), (0, -0.5), (1, -0.5), (0, -1.5)):
            ink_parts.append(ink(Rot(0, 0, 45) * Rectangle(s * 0.95, s * 0.6), (x + y * 0.3) * s, v + y * s * 0.8))
    elif logo and logo[0] == 'motorola':
        # Motorola's batwing: two pointed arches in a ring, tapering to points at the bottom
        _, u, v, r = logo
        def bezier(p, c, q, n=8):
            return [((1 - t) ** 2 * p[0] + 2 * (1 - t) * t * c[0] + t * t * q[0],
                     (1 - t) ** 2 * p[1] + 2 * (1 - t) * t * c[1] + t * t * q[1]) for t in (i / n for i in range(n))]
        tip, peak, notch, under, vee = (-0.56, -0.46), (-0.27, 0.48), (0, 0.02), (-0.26, 0.2), (0, -0.44)
        outer = bezier(tip, (-0.6, 0.3), peak) + bezier(peak, (-0.1, 0.5), notch)
        inner = bezier(tip, (-0.48, 0.12), under)[1:] + bezier(under, (-0.1, 0.2), vee)
        mirror = [(-x, y) for x, y in outer[::-1] + inner]
        wing = outer + [notch] + mirror + [vee] + inner[::-1]
        ink_parts += [ink(Circle(r) - Circle(r * 0.87), u, v),
                      ink(Polygon(*[(x * r, y * r) for x, y in reversed(wing)], align=None), u, v)]
    elif logo:
        ch, v, r = (tuple(logo) + (0.9,))[:3]
        ink_parts += ringed(ch, 0, v, r)
    for ch, u, v, r in circled:
        ink_parts += ringed(ch, u, v, r)
    ink_parts += [ink(Circle(r), u, v) for u, v, r in dots]
    ink_parts += [ink(Rectangle(w, h), u, v) for u, v, w, h in rects]
    b.label, b.color = 'body', Color(0.17, 0.16, 0.15)
    leads.label, leads.color = 'lead', Color(0.85, 0.85, 0.83)
    children = [b, leads]
    if ink_parts:
        mark = Compound(ink_parts)
        mark.label, mark.color = 'mark', Color(0.72, 0.69, 0.60)
        children.append(mark)
    model = Compound(label=label, children=children)
    export_step(model, out)
    return model
