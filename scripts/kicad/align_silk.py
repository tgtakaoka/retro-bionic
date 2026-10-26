"""Tidy a routed board's silkscreen reference labels.

    align_silk.py <board.kicad_pcb>

Two steps:

- align: footprints of one kind (same library footprint, side and
  orientation) take the reference angle most of them use and the median
  offset of those labels; a label within SNAP of that spot moves onto it
  where the spot is clear of pads, other silkscreen and the edge, so
  labels placed elsewhere on purpose stay.
- clear: a reference or footprint text that DRC reports on silkscreen,
  solder mask or the board edge moves to the nearest clear spot around
  its own position, trying its angle and the angle turned 90 degrees.

The board is checked with kicad-cli DRC before and after; if the count
grows, it is left as it was. Saves in place, so close it in KiCad first.
"""
import math
import os
import re
import subprocess
import sys
from collections import Counter, defaultdict

import pcbnew

MM = 1e6
PAD_GAP = 0.12      # clearance kept from pads (solder mask openings)
SILK_GAP = 0.1      # clearance kept from other silkscreen
EDGE_GAP = 0.35     # clearance kept inside the board outline
SEARCH = 4.5        # how far the clear step looks for a spot
SNAP = 1.0          # labels this close to their kind's spot move onto it
SILK = (pcbnew.F_SilkS, pcbnew.B_SilkS)


def drc(path):
    """(violation count, [(kind, ref, text)] of silkscreen items DRC names)"""
    rpt = path + '.silk.rpt'
    subprocess.run(['kicad-cli', 'pcb', 'drc', '-o', rpt, path], capture_output=True)
    text = open(rpt).read()
    os.remove(rpt)
    count = sum(int(n) for n in re.findall(r'Found (\d+) (?:DRC violations|unconnected pads)', text))
    items = set()
    for block in text.split('\n['):
        if not block.startswith(('silk_overlap', 'silk_over_copper', 'silk_edge_clearance')):
            continue
        for m in re.finditer(r'(Reference field|Footprint text) of (\S+)(?: \((.*?)\))?', block):
            items.add((m.group(1), m.group(2), m.group(3)))
    return count, sorted(items, key=str)


def side(layer):
    return 'F' if layer in (pcbnew.F_SilkS, pcbnew.F_Cu) else 'B'


class Board:
    def __init__(self, b):
        self.b = b
        self.edge = b.GetBoardEdgesBoundingBox()
        self.edge.Inflate(-int(EDGE_GAP * MM))

    def obstacles(self, me, s):
        out = []
        cu = pcbnew.F_Cu if s == 'F' else pcbnew.B_Cu
        for f in self.b.GetFootprints():
            for p in f.Pads():
                if p.IsOnLayer(cu):
                    bb = p.GetBoundingBox()
                    bb.Inflate(int(PAD_GAP * MM))
                    out.append(bb)
            for g in list(f.GraphicalItems()) + [f.Reference(), f.Value()]:
                if g is me or g.GetLayer() not in SILK or side(g.GetLayer()) != s:
                    continue
                if hasattr(g, 'IsVisible') and not g.IsVisible():
                    continue
                bb = g.GetBoundingBox()
                bb.Inflate(int(SILK_GAP * MM))
                out.append(bb)
        for d in self.b.GetDrawings():
            if d is not me and d.GetLayer() in SILK and side(d.GetLayer()) == s:
                bb = d.GetBoundingBox()
                bb.Inflate(int(SILK_GAP * MM))
                out.append(bb)
        return out

    def clear(self, t, obs):
        bb = t.GetBoundingBox()
        if not (self.edge.Contains(bb.GetOrigin()) and self.edge.Contains(bb.GetEnd())):
            return False
        return not any(bb.Intersects(o) for o in obs)


def kind(f):
    return (str(f.GetFPID().GetLibItemName()), side(f.GetLayer()), round(f.GetOrientationDegrees()) % 360)


def offset(f):
    t, p = f.Reference(), f.GetPosition()
    return (round((t.GetPosition().x - p.x) / MM, 2), round((t.GetPosition().y - p.y) / MM, 2),
            round(t.GetTextAngleDegrees()) % 360)


def median(v):
    v = sorted(v)
    return v[len(v) // 2] if len(v) % 2 else (v[len(v) // 2 - 1] + v[len(v) // 2]) / 2


def align(bd):
    """each footprint kind's labels near its common spot move onto it where clear"""
    groups = defaultdict(list)
    for f in bd.b.GetFootprints():
        if f.Reference().IsVisible() and f.Reference().GetLayer() in SILK:
            groups[kind(f)].append(f)
    moved = []
    for fs in groups.values():
        if len(fs) < 2:
            continue
        (angle, n), = Counter(offset(f)[2] for f in fs).most_common(1)
        if n < 2:
            continue
        same = [offset(f) for f in fs if offset(f)[2] == angle]
        best = (round(median([o[0] for o in same]), 2), round(median([o[1] for o in same]), 2), angle)
        for f in fs:
            o = offset(f)
            if o == best or math.hypot(o[0] - best[0], o[1] - best[1]) > SNAP:
                continue
            t = f.Reference()
            moved.append((f.GetReference(), t.GetPosition(), t.GetTextAngleDegrees()))
            p = f.GetPosition()
            t.SetTextAngleDegrees(best[2])
            t.SetPosition(pcbnew.VECTOR2I(p.x + int(round(best[0] * MM)), p.y + int(round(best[1] * MM))))
    return moved


def undo(b, moved, refs):
    """puts the labels of refs back where align found them"""
    for ref, pos, ang in moved:
        if ref in refs:
            t = b.FindFootprintByReference(ref).Reference()
            t.SetTextAngleDegrees(ang)
            t.SetPosition(pos)
    return [m for m in moved if m[0] not in refs]


def clear_flagged(bd, items):
    """moves each DRC-flagged text to the nearest clear spot"""
    moved, stuck = [], []
    for what, ref, txt in items:
        f = bd.b.FindFootprintByReference(ref)
        if f is None:
            continue
        if what == 'Reference field':
            t = f.Reference()
        else:
            ts = [g for g in f.GraphicalItems() if g.GetClass() == 'PCB_TEXT' and g.GetText() == txt]
            if not ts:
                continue
            t = ts[0]
        obs = bd.obstacles(t, side(t.GetLayer()))
        p0, a0 = t.GetPosition(), t.GetTextAngleDegrees()
        angles = [a0] if what != 'Reference field' else [a0, (a0 + 90) % 180]
        found = None
        for r in [k * 0.1 for k in range(1, int(SEARCH * 10) + 1)]:
            for ang in angles:
                for k in range(16):
                    th = 2 * math.pi * k / 16
                    t.SetTextAngleDegrees(ang)
                    t.SetPosition(pcbnew.VECTOR2I(p0.x + int(r * math.cos(th) * MM), p0.y + int(r * math.sin(th) * MM)))
                    if bd.clear(t, obs):
                        found = True
                        break
                if found:
                    break
            if found:
                break
        if found:
            moved.append(ref if txt is None else '%s(%s)' % (ref, txt))
        else:
            t.SetTextAngleDegrees(a0)
            t.SetPosition(p0)
            stuck.append(ref)
    return moved, stuck


def main(path):
    path = os.path.abspath(path)
    original = open(path, 'rb').read()
    count, items = drc(path)
    b = pcbnew.LoadBoard(path)
    bd = Board(b)
    flagged = {r for _, r, _ in items}
    moved = align(bd)
    # DRC judges the aligned spots: a label it flags that was clean before goes back
    for _ in range(5):
        if not moved:
            break
        pcbnew.SaveBoard(path, b)
        _, items = drc(path)
        bad = {r for _, r, _ in items} & {m[0] for m in moved} - flagged
        if not bad:
            break
        moved = undo(b, moved, bad)
    aligned = [m[0] for m in moved]
    if aligned:
        pcbnew.SaveBoard(path, b)
        _, items = drc(path)
        b = pcbnew.LoadBoard(path)
        bd = Board(b)
    moved, stuck = clear_flagged(bd, items)
    if moved:
        pcbnew.SaveBoard(path, b)
    if not aligned and not moved:
        open(path, 'wb').write(original)
        print('nothing to move, DRC %d' % count)
        sys.stdout.flush()
        os._exit(0)
    after, left = drc(path)
    if after > count:
        open(path, 'wb').write(original)
        print('DRC grew from %d to %d: board left as it was' % (count, after))
    else:
        print('aligned %d labels, cleared %d, DRC %d -> %d' % (len(aligned), len(moved), count, after))
        if aligned:
            print('  aligned: ' + ' '.join(aligned))
        if moved:
            print('  cleared: ' + ' '.join(moved))
        if stuck or left:
            print('  still flagged: ' + ' '.join(sorted({r for _, r, _ in left})))
    sys.stdout.flush()
    os._exit(0)     # pcbnew can crash tearing down a modified board


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
