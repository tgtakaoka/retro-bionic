"""STEP model of a square gull-wing QFP with its top marking, for a footprint's 3D view.

Needs build123d (not in KiCad's Python): run with the CAD venv, e.g.
~/.venvs/cad/bin/python. Dimensions come from the package drawing; the
marking lines and mold marks from a photo of the part.
"""
from build123d import (Align, Axis, Box, Circle, Color, Compound, Cylinder, FontStyle, Pos, Rectangle, Rot, Text,
                       chamfer, export_step, extrude)

PIN1_DEPTH = 0.3    # the pin 1 dimple's depth


def qfp(out, label, body, span, pitch, per_side, lead_width, lead_thick, foot, standoff, thick,
        lines=(), logo=None, pin1=None, marks=(), dots=()):
    """lines: [(text, v, font_size)] centred, v up from the body centre, read with pin 1 at the lower left
    logo: (letter, v) or (letter, v, ring radius) in a ring, or ('freescale', v, width) for its tiles
    pin1: (u, v, radius) the pin 1 dimple, PIN1_DEPTH deep; marks: [(u, v, radius, depth)] other round
    dimples, e.g. ejector pin marks; dots: [(u, v, radius)] printed dots"""
    zmid = standoff + thick / 2
    top = standoff + thick
    b = Box(body, body, thick, align=(Align.CENTER, Align.CENTER, Align.MIN))
    b = Pos(0, 0, standoff) * chamfer(b.edges().group_by(Axis.Z)[-1], 0.2)
    for u, v, r, depth in ([pin1 + (PIN1_DEPTH,)] if pin1 else []) + list(marks):
        b = b - Rot(0, 0, -90) * (Pos(u, v, top - depth) *
                                  Cylinder(r, depth + 0.05, align=(Align.CENTER, Align.CENTER, Align.MIN)))

    # gull-wing lead pointing to -X, centred on y = 0
    xe = -body / 2
    xt = -span / 2
    xk = xt + foot                 # heel
    xb = xe - 0.25                 # shoulder
    upper = Pos((xe + 0.1 + xb) / 2, 0, zmid) * Box(xe + 0.1 - xb, lead_width, lead_thick)
    leg = Pos((xb + xk) / 2, 0, (zmid + lead_thick / 2) / 2) * Box(abs(xb - xk) + lead_thick, lead_width,
                                                                   zmid + lead_thick / 2)
    sole = Pos((xk + xt) / 2, 0, lead_thick / 2) * Box(xk - xt, lead_width, lead_thick)
    lead = upper + leg + sole
    leads = Compound([Rot(0, 0, 90 * side) * (Pos(0, (per_side - 1) / 2 * pitch - i * pitch, 0) * lead)
                      for side in range(4) for i in range(per_side)])

    def ink(shape, u, v):
        return Rot(0, 0, -90) * (Pos(u, v, top) * extrude(shape, 0.01))

    ink_parts = [ink(Text(t, font_size=size), 0, v) for t, v, size in lines]
    if logo and logo[0] == 'freescale':
        # Freescale's mark: slanted tiles stacked in a diamond, size wide
        _, v, size = logo
        s = size / 4
        for x, y in ((0, 1.5), (-0.5, 0.5), (0.5, 0.5), (-1, -0.5), (0, -0.5), (1, -0.5), (0, -1.5)):
            ink_parts.append(ink(Rot(0, 0, 45) * Rectangle(s * 0.95, s * 0.6), (x + y * 0.3) * s, v + y * s * 0.8))
    elif logo:
        ch, v, r = (tuple(logo) + (0.9,))[:3]
        ink_parts += [ink(Circle(r) - Circle(r * 0.87), 0, v),
                      ink(Text(ch, font_size=r * 1.1, font_style=FontStyle.BOLD), 0, v)]
    ink_parts += [ink(Circle(r), u, v) for u, v, r in dots]
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
