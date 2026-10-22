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
        lines=(), logo=None, pin1=None, marks=(), dots=(), rects=(), circled=(), font=None):
    """body, span (lead tip to tip) and per_side (leads on each side) are one value for a square
    package, or (x, y) in the footprint's frame for a rectangular one: per_side[0] leads on each
    X side, the side pin 1 starts.
    Marking positions (u, v) are mm from the body centre as read with pin 1 at the lower left:
    lines: [(text, v, font_size)] centred, or [(text, v, font_size, u, width)] from u at the
    left, its glyphs spread evenly over width
    logo: (letter, v) or (letter, v, ring radius) in a ring, or ('freescale', v, width) for its tiles
    pin1: (u, v, radius) the pin 1 dimple, PIN1_DEPTH deep; marks: [(u, v, radius, depth)] other round
    dimples, e.g. ejector pin marks; dots: [(u, v, radius)] printed dots; rects: [(u, v, width,
    height)] printed rectangles, centred; circled: [(letter, u, v, ring radius)], e.g. (M) and (C)
    font: a font file for the text, else build123d's own"""
    body_x, body_y = pair(body)
    span_x, span_y = pair(span)
    count_x, count_y = pair(per_side)
    zmid = standoff + thick / 2
    top = standoff + thick
    b = Box(body_x, body_y, thick, align=(Align.CENTER, Align.CENTER, Align.MIN))
    b = Pos(0, 0, standoff) * chamfer(b.edges().group_by(Axis.Z)[-1], 0.2)
    for u, v, r, depth in ([pin1 + (PIN1_DEPTH,)] if pin1 else []) + list(marks):
        b = b - Rot(0, 0, -90) * (Pos(u, v, top - depth) *
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
        return Rot(0, 0, -90) * (Pos(u, v, top) * extrude(shape, 0.01))

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
