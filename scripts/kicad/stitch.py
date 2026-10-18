"""Tie a routed board's GND pours together.

    stitch.py <board.kicad_pcb> [drc.rpt]

Every GND pour island without a via or through-hole pad gets a via near
its middle. With a kicad-cli DRC report, each GND pad it names as
unconnected or with a starved thermal gets a short stub to its own via.
Vias take the Default netclass size; the board is refilled and saved.
"""
import math
import os
import re
import sys

import pcbnew

V = pcbnew.VECTOR2I_MM
mm = pcbnew.ToMM


def seg_dist(p, a, c):
    (px, py), (ax, ay), (cx, cy) = p, a, c
    dx, dy = cx - ax, cy - ay
    n = dx * dx + dy * dy
    t = 0 if n == 0 else max(0, min(1, ((px - ax) * dx + (py - ay) * dy) / n))
    return math.hypot(px - ax - t * dx, py - ay - t * dy)


class Stitcher:
    def __init__(self, b, net='GND'):
        self.b = b
        self.net = b.FindNet(net)
        nc = b.GetDesignSettings().m_NetSettings.GetDefaultNetclass()
        self.via_d, self.via_h = mm(nc.GetViaDiameter()), mm(nc.GetViaDrill())
        self.clr = mm(nc.GetClearance())
        self.track = mm(nc.GetTrackWidth())
        bb = b.GetBoardEdgesBoundingBox()
        self.box = mm(bb.GetLeft()), mm(bb.GetTop()), mm(bb.GetRight()), mm(bb.GetBottom())
        self.obs = []      # copper of other nets on any layer: ('box', rect, 0) or ('seg', (a, c), half width)
        for p in b.GetPads():
            if p.GetNetCode() != self.net.GetNetCode():
                r = p.GetBoundingBox()
                self.obs.append(('box', (mm(r.GetLeft()), mm(r.GetTop()), mm(r.GetRight()), mm(r.GetBottom())), 0))
        for t in b.GetTracks():
            if t.GetNetCode() != self.net.GetNetCode():
                s, e = t.GetStart(), t.GetEnd()
                w = mm(t.GetWidth(pcbnew.F_Cu)) if t.Type() == pcbnew.PCB_VIA_T else mm(t.GetWidth())
                self.obs.append(('seg', ((mm(s.x), mm(s.y)), (mm(e.x), mm(e.y))), w / 2))

    def _clear(self, pt, r):
        x, y = pt
        for kind, g, hw in self.obs:
            if kind == 'box':
                x0, y0, x1, y1 = g
                if x0 - r < x < x1 + r and y0 - r < y < y1 + r:
                    return False
            elif seg_dist(pt, *g) < r + hw:
                return False
        return True

    def free(self, pt):
        x0, y0, x1, y1 = self.box
        if not (x0 + 0.5 < pt[0] < x1 - 0.5 and y0 + 0.5 < pt[1] < y1 - 0.5):
            return False
        return self._clear(pt, self.via_d / 2 + self.clr)

    def path_free(self, a, c, hw):
        n = max(2, int(math.dist(a, c) / 0.05))
        return all(self._clear((a[0] + (c[0] - a[0]) * k / n, a[1] + (c[1] - a[1]) * k / n), hw + self.clr)
                   for k in range(1, n))

    def via(self, pt):
        v = pcbnew.PCB_VIA(self.b)
        v.SetPosition(V(*pt))
        v.SetDrill(pcbnew.FromMM(self.via_h))
        v.SetWidth(pcbnew.FromMM(self.via_d))
        v.SetNet(self.net)
        self.b.Add(v)
        self.obs.append(('seg', (pt, pt), self.via_d / 2))   # keep later vias apart from this one

    def anchors(self):
        code = self.net.GetNetCode()
        pts = [t.GetPosition() for t in self.b.GetTracks() if t.Type() == pcbnew.PCB_VIA_T and t.GetNetCode() == code]
        pts += [p.GetPosition() for p in self.b.GetPads()
                if p.GetNetCode() == code and p.GetAttribute() == pcbnew.PAD_ATTRIB_PTH]
        return pts

    def _outlines(self):
        """[(layer, inside(point), area, bbox)] of every filled GND piece; inside respects its holes"""
        out = []
        for z in self.b.Zones():
            if z.GetNetCode() == self.net.GetNetCode():
                for layer in z.GetLayerSet().Seq():
                    polys = z.GetFilledPolysList(layer)
                    for i in range(polys.OutlineCount()):
                        ol = polys.Outline(i)
                        out.append((layer, lambda p, ps=polys, k=i: ps.Contains(p, k), ol.Area(), ol.BBox()))
        return out

    def _groups(self, pieces):
        """pieces joined through vias and through-hole pads; the largest group first"""
        parent = list(range(len(pieces)))

        def root(i):
            while parent[i] != i:
                parent[i] = parent[parent[i]]
                i = parent[i]
            return i

        for a in self.anchors():
            hit = [k for k, piece in enumerate(pieces) if piece[1](a)]
            for k in hit[1:]:
                parent[root(k)] = root(hit[0])
        groups = {}
        for k in range(len(pieces)):
            groups.setdefault(root(k), []).append(k)
        return sorted(groups.values(), key=lambda g: -sum(pieces[k][2] for k in g))

    def islands(self):
        """join every pour group to the largest one with a via where they overlap"""
        added = 0
        filler = pcbnew.ZONE_FILLER(self.b)
        for _ in range(8):
            filler.Fill(self.b.Zones())
            pieces = self._outlines()
            groups = self._groups(pieces)
            if len(groups) < 2:
                break
            main = [pieces[k] for k in groups[0]]
            new = 0
            for g in groups[1:]:
                for k in g:
                    layer, inside, _, bx = pieces[k]
                    cands = []
                    x = mm(bx.GetLeft())
                    while x <= mm(bx.GetRight()):
                        y = mm(bx.GetTop())
                        while y <= mm(bx.GetBottom()):
                            p = V(x, y)
                            if inside(p) and any(m[0] != layer and m[1](p) for m in main) \
                                    and self.free((x, y)):
                                cands.append((round(x, 3), round(y, 3)))
                            y += 0.1
                        x += 0.1
                    if cands:
                        cx = sum(c[0] for c in cands) / len(cands)
                        cy = sum(c[1] for c in cands) / len(cands)
                        self.via(min(cands, key=lambda c: math.hypot(c[0] - cx, c[1] - cy)))
                        new += 1
                        break
                    if self._reach(layer, inside, bx, main):
                        new += 1
                        break
                else:
                    print('no via spot for a pour piece')
            added += new
            if not new:
                break
        return added

    def _reach(self, layer, inside, bx, main):
        """no via fits in the piece: run a short track from it to a via over the main pour"""
        pts = []
        x = mm(bx.GetLeft())
        while x <= mm(bx.GetRight()):
            y = mm(bx.GetTop())
            while y <= mm(bx.GetBottom()):
                if inside(V(x, y)):
                    pts.append((round(x, 3), round(y, 3)))
                y += 0.1
            x += 0.1
        best = None
        for r10 in range(int(self.via_d * 10) + 1, 30, 2):      # nearest via spot first
            for p in pts[::2]:
                for k in range(16):
                    a = 2 * math.pi * k / 16
                    q = (round(p[0] + r10 / 10 * math.cos(a), 3), round(p[1] + r10 / 10 * math.sin(a), 3))
                    if self.free(q) and any(m[0] != layer and m[1](V(*q)) for m in main) \
                            and self.path_free(p, q, self.track / 2):
                        best = (r10 / 10, p, q)
                        break
                if best:
                    break
            if best:
                break
        if not best:
            return False
        _, p, q = best
        t = pcbnew.PCB_TRACK(self.b)
        t.SetStart(V(*p))
        t.SetEnd(V(*q))
        t.SetWidth(pcbnew.FromMM(self.track))
        t.SetLayer(layer)
        t.SetNet(self.net)
        self.b.Add(t)
        self.via(q)
        self.obs.append(('seg', (p, q), 0))
        print('track %s -> via %s' % (p, q))
        return True

    def pad_stub(self, pad):
        px, py = mm(pad.GetPosition().x), mm(pad.GetPosition().y)
        w = self.track
        for r10 in range(int(self.via_d * 10) + 1, 30):
            for k in range(24):
                a = 2 * math.pi * k / 24
                pt = (round(px + r10 / 10 * math.cos(a), 3), round(py + r10 / 10 * math.sin(a), 3))
                if self.free(pt) and self.path_free((px, py), pt, w / 2):
                    t = pcbnew.PCB_TRACK(self.b)
                    t.SetStart(V(px, py))
                    t.SetEnd(V(*pt))
                    t.SetWidth(pcbnew.FromMM(w))
                    t.SetLayer(pcbnew.F_Cu if pad.IsOnLayer(pcbnew.F_Cu) else pcbnew.B_Cu)
                    t.SetNet(self.net)
                    self.b.Add(t)
                    self.via(pt)
                    self.obs.append(('seg', ((px, py), pt), 0))
                    return pt
        return None


def main(path, rpt=None):
    b = pcbnew.LoadBoard(path)
    s = Stitcher(b)
    if rpt:
        done = set()
        for num, ref in re.findall(r'Pad (\w+) \[GND\] of (\w+)', open(rpt).read()):
            if (ref, num) in done:
                continue
            done.add((ref, num))
            pad = [p for p in b.FindFootprintByReference(ref).Pads() if p.GetNumber() == num][0]
            print('stub %s.%s -> %s' % (ref, num, s.pad_stub(pad)))
    print('island vias: %d' % s.islands())
    pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    pcbnew.SaveBoard(path, b)


if __name__ == '__main__':
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    main(*sys.argv[1:])
    sys.stdout.flush()
    os._exit(0)       # pcbnew can crash tearing down a board it added items to
