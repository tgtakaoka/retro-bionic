"""Straighten a routed board's tracks.

    straighten_tracks.py <board.kicad_pcb>

Cleans up router and hand-routing leftovers in three steps, repeated
until nothing changes:

- snap: the ends of off-axis segments that stop a few microns short of a
  via centre or of another track end are joined to it, and a segment
  shorter than SHORT collapses onto its stronger end (via, pad, the
  busier junction). Longer segments at 0, 45 or 90 degrees are
  deliberate and keep their ends.
- joints: a via or junction whose tracks meet slightly off-axis moves
  by up to SLIDE to where all of them run at 0, 45 or 90 degrees. A via
  between stubs shorter than LOOK on two layers moves to their corner.
- runs: each run of track between anchors (pads, vias, junctions) is
  rebuilt from horizontal, vertical and 45-degree segments, dropping
  micro jogs: short jogs, the ends inside pads (along the pad's centre
  line) and vias free to slide along their other tracks take up the
  offset, long runs stay put. A pad end off its pad's centre line is
  moved onto it. A run that fits only with its via moved across the
  via's other tracks gets it, those tracks rebuilt with it.

Segments of no length, inside one pad or lying on another track go too.
Each pass is checked with kicad-cli DRC against the board as it came; a
change that adds a violation, or leaves a track at an odd angle, is
undone and reported. A change whose only violation is another net's via
is tried again with that via moved aside by up to SLIDE. The board is
refilled and saved in place, so close it in KiCad first; on an error it
is left as it was.
"""
import math
import os
import re
import subprocess
import sys
from collections import defaultdict

import pcbnew

SHORT = 0.1       # segments shorter than this are jogs to absorb
SLIDE = 0.12      # how far a length, a pad end or a joint may move
NEAR = 0.03       # ends this close are one point
LOOK = 0.1        # a via between stubs this short on two layers belongs at their corner
V = pcbnew.VECTOR2I_MM
mm = pcbnew.ToMM
DIRS = [(math.cos(k * math.pi / 4), math.sin(k * math.pi / 4)) for k in range(8)]


def P(v):
    return (mm(v.x), mm(v.y))


def key(p):
    return (round(p[0], 4), round(p[1], 4))


def on_axis(p, q):
    dx, dy = abs(q[0] - p[0]), abs(q[1] - p[1])
    return dx < 1e-4 or dy < 1e-4 or abs(dx - dy) < 1e-4


def dist(p, q):
    return math.hypot(p[0] - q[0], p[1] - q[1])


class Straightener:
    def __init__(self, b):
        self.b = b
        self.pads = list(b.GetPads())
        self.vias = {key(P(v.GetPosition())): v for v in b.GetTracks() if v.Type() == pcbnew.PCB_VIA_T}
        self.touched = set()     # vias whose tracks changed this pass
        self.ends = defaultdict(list)
        for t in self.segs():
            for i, e in enumerate((t.GetStart(), t.GetEnd())):
                self.ends[(t.GetNetCode(), t.GetLayer(), key(P(e)))].append((t, i))

    def segs(self):
        return [t for t in self.b.GetTracks() if t.Type() == pcbnew.PCB_TRACE_T]

    def set_end(self, t, i, p):
        (t.SetStart if i == 0 else t.SetEnd)(V(*p))

    def snap(self, skip):
        """join off-axis segments' ends to the via centre or track end they stop short of"""
        vias = [(v.GetNetCode(), k, mm(v.GetWidth(pcbnew.F_Cu)) / 2) for k, v in self.vias.items()]
        points = defaultdict(set)
        for n, L, k in self.ends:
            points[(n, L)].add(k)
        done = {}
        for t in self.segs():
            a, c = key(P(t.GetStart())), key(P(t.GetEnd()))
            if on_axis(a, c) and dist(a, c) >= SHORT:
                continue
            n, L = t.GetNetCode(), t.GetLayer()
            for i, p in enumerate((a, c)):
                if p in self.vias or ('snap', n, p) in skip:
                    continue
                to = [k for vn, k, r in vias if vn == n and dist(p, k) < r]
                if not to:
                    near = [q for q in points[(n, L)] if q != p and dist(p, q) < NEAR]
                    # the busiest point wins, so a crowd of ends meets at one
                    to = sorted(near, key=lambda q: (-len(self.ends[(n, L, q)]), q))[:1]
                q = c if i == 0 else a
                if not to and dist(a, c) < SHORT and self.weaker(n, L, p, q):
                    # a segment this short is noise: its weaker end joins the other
                    to = [q]
                if to and ('snap', n, p) not in done:
                    # every end at the point goes, or the others would be left hanging
                    segs = []
                    for u, j in self.ends[(n, L, p)]:
                        self.set_end(u, j, to[0])
                        segs.append((P(u.GetStart()), P(u.GetEnd())))
                    done[('snap', n, p)] = (t.GetNetname(), segs)
        return done

    def weaker(self, n, L, p, q):
        """p may move onto q: q holds a via, pad or more ends, and p's pad covers q"""
        pp, pq = self.pad_at(p, n, L), self.pad_at(q, n, L)
        if pp and not pp.HitTest(V(*q)):
            return False
        rank = lambda k, pad: (k in self.vias, pad is not None, len(self.ends[(n, L, k)]), k)
        return rank(p, pp) < rank(q, pq)

    def joints(self, skip):
        """move vias and junctions whose tracks meet off-axis to where all run on axis"""
        nodes = defaultdict(list)
        for (n, L, k), e in self.ends.items():
            node = (n, 'via' if k in self.vias else L, k)
            for t, i in e:
                q = key(P(t.GetEnd() if i == 0 else t.GetStart()))
                # a pad end counts where the runs step will put it, on the pad's centre line
                nodes[node].append((t, i, self.centred(q, self.pad_at(q, n, L)), None))
        for (n, L, p), lst in nodes.items():
            if L == 'via':
                lst[:] = self.through_stubs(n, p, lst)
        done, frozen = {}, set()
        for (n, L, p), lst in nodes.items():
            if (('joint', n, p) in skip or p in frozen or all(on_axis(p, q) for _, _, q, _ in lst)
                    or any(pd.GetNetCode() == n and pd.HitTest(V(*p)) for pd in self.pads)):
                continue
            qs = sorted({q for _, _, q, _ in lst})
            cands = []
            for q in qs:
                for d in DIRS[:4]:
                    # p moved straight across onto q's line along d
                    u = (p[0] - q[0]) * d[0] + (p[1] - q[1]) * d[1]
                    cands.append((q[0] + u * d[0], q[1] + u * d[1]))
            for i1, q1 in enumerate(qs):
                for q2 in qs[i1 + 1:]:
                    for d1 in DIRS[:4]:
                        for d2 in DIRS[:4]:
                            det = d2[0] * d1[1] - d1[0] * d2[1]
                            if abs(det) > 1e-9:
                                u = ((q2[0] - q1[0]) * -d2[1] + (q2[1] - q1[1]) * d2[0]) / det
                                cands.append((q1[0] + u * d1[0], q1[1] + u * d1[1]))
            cands = sorted((key(c) for c in cands if 1e-5 < dist(c, p) <= SLIDE), key=lambda c: dist(c, p))
            to = next((c for c in cands if all(on_axis(c, q) and dist(c, q) > 1e-3 for _, _, q, _ in lst)), None)
            if not to:
                continue
            if L == 'via':
                self.vias[p].SetPosition(V(*to))
            segs = [(to, to)]
            for t, i, q, stub in lst:
                self.set_end(t, i, to)
                if stub:
                    # the stub shrinks to nothing and drop_idle takes it
                    stub.SetStart(V(*to))
                    stub.SetEnd(V(*to))
                segs.append((to, q))
                frozen.add(q)
            frozen.add(p)
            done[('joint', n, p)] = (lst[0][0].GetNetname(), segs)
        return done

    def through_stubs(self, n, p, lst):
        """a via between short stubs on two layers: see past each stub to the track it turns into"""
        stubs = [(t, i, q) for t, i, q, _ in lst if dist(p, q) < LOOK and not self.pad_at(q, n, t.GetLayer())]
        if len({t.GetLayer() for t, _, _ in stubs}) < 2:
            return lst
        out = [e for e in lst if not any(e[0] is t for t, _, _ in stubs)]
        for t, i, q in stubs:
            nxt = [(u, j) for u, j in self.ends[(n, t.GetLayer(), q)] if u is not t]
            if len(nxt) != 1:
                return lst
            u, j = nxt[0]
            out.append((u, j, key(P(u.GetEnd() if j == 0 else u.GetStart())), t))
        return out

    def pad_at(self, pt, net, layer):
        for p in self.pads:
            if p.GetNetCode() == net and p.IsOnLayer(layer) and p.HitTest(V(*pt)):
                return p

    def anchor(self, net, layer, k):
        return k in self.vias or len(self.ends[(net, layer, k)]) != 2 or self.pad_at(k, net, layer)

    def chains(self):
        """[(net, layer, [start, (track, point), ...])] between anchors"""
        done, out = set(), []
        for (n, L, k), e in list(self.ends.items()):
            if not self.anchor(n, L, k):
                continue
            for t, i in e:
                if id(t) in done:
                    continue
                chain = [k]
                while True:
                    done.add(id(t))
                    k = key(P(t.GetEnd() if i == 0 else t.GetStart()))
                    chain.append((t, k))
                    if self.anchor(n, L, k):
                        break
                    t, i = [(u, j) for u, j in self.ends[(n, L, k)] if u is not t][0]
                out.append((n, L, chain))
                k = chain[0]
        return out

    @staticmethod
    def solve(A, B, dirs, l0, slackA, slackB):
        """lengths along dirs and pad-end offsets closing A..B, least weighted change"""
        cols, w = [], []
        for d, l in zip(dirs, l0):
            cols.append(d)
            w.append(1.0 + 100 * l * l)      # long runs stay put
        for s, sign in ((slackA, 1), (slackB, -1)):
            if s:
                cols.append((sign * s[0], sign * s[1]))
                w.append(0.3)
        r = (B[0] - A[0] - sum(l * d[0] for l, d in zip(l0, dirs)),
             B[1] - A[1] - sum(l * d[1] for l, d in zip(l0, dirs)))
        # a tiny ridge keeps runs along one axis solvable; an offset across them stays open
        m = [[sum(c[a] * c[e] / wi for c, wi in zip(cols, w)) + 1e-9 * (a == e) for e in (0, 1)] for a in (0, 1)]
        det = m[0][0] * m[1][1] - m[0][1] * m[1][0]
        if abs(det) < 1e-12:
            return None
        lam = ((m[1][1] * r[0] - m[0][1] * r[1]) / det, (m[0][0] * r[1] - m[1][0] * r[0]) / det)
        x = [(c[0] * lam[0] + c[1] * lam[1]) / wi for c, wi in zip(cols, w)]
        l = [a + dx for a, dx in zip(l0, x[:len(dirs)])]
        rest = x[len(dirs):]
        eA = (rest[0] * slackA[0], rest[0] * slackA[1]) if slackA else (0, 0)
        eB = (rest[-1] * slackB[0], rest[-1] * slackB[1]) if slackB else (0, 0)
        return l, eA, eB

    @staticmethod
    def slack(p):
        """a pad end slides along the pad only, keeping to its centre line"""
        if not p:
            return None
        bb = p.GetBoundingBox()
        return (1, 0) if bb.GetWidth() > bb.GetHeight() else (0, 1)

    def via_slack(self, k, net, chain):
        """the one direction all the via's other tracks run in: it may slide along them"""
        if k not in self.vias or k in self.touched or any(
                p.GetNetCode() == net and p.HitTest(V(*k)) for p in self.pads):
            return None
        dirs = set()
        for L in self.b.GetEnabledLayers().CuStack():
            for t, i in self.ends.get((net, L, k), []):
                if t in chain:
                    continue
                q = P(t.GetEnd() if i == 0 else t.GetStart())
                v = (q[0] - k[0], q[1] - k[1])
                if math.hypot(*v) < 2 * SLIDE or not on_axis(k, q):
                    return None
                d = max(DIRS, key=lambda r: abs(r[0] * v[0] + r[1] * v[1]))
                dirs.add(d)
        return dirs.pop() if len(dirs) == 1 else None

    def slide(self, k, to, net):
        """move a via and its tracks' ends; returns the tracks moved"""
        self.vias[k].SetPosition(V(*to))
        moved = [(to, to)]
        for t in self.segs():
            if t.GetNetCode() == net:
                for e, s in ((t.GetStart(), t.SetStart), (t.GetEnd(), t.SetEnd)):
                    if key(P(e)) == k:
                        s(V(*to))
                        moved.append((P(t.GetStart()), P(t.GetEnd())))
        return moved

    def centred(self, pt, p):
        """a pad end moved onto the pad's centre line"""
        if not p:
            return pt
        c = P(p.GetPosition())
        return key((pt[0], c[1]) if self.slack(p) == (1, 0) else (c[0], pt[1]))

    def plan(self, n, L, chain, moved={}, slacks={}):
        """the rebuilt run for a chain, 'left' if none fits, None if it is fine as it is

        moved puts a via end at a new place, held there; slacks lets a via end
        slide along the given direction."""
        A0, B0 = chain[0], chain[-1][1]
        pts = [A0] + [k for _, k in chain[1:]]
        tracks = [t for t, _ in chain[1:]]
        dirs, l0, wid = [], [], []
        for p, q, t in zip(pts, pts[1:], tracks):
            v = (q[0] - p[0], q[1] - p[1])
            if math.hypot(*v) < SHORT:
                continue
            d = max(DIRS, key=lambda r: r[0] * v[0] + r[1] * v[1])
            if dirs and dirs[-1] == d:
                l0[-1] += d[0] * v[0] + d[1] * v[1]
                wid[-1] = max(wid[-1], t.GetWidth())
            else:
                dirs.append(d)
                l0.append(d[0] * v[0] + d[1] * v[1])
                wid.append(t.GetWidth())
        pa = None if A0 in self.vias else self.pad_at(A0, n, L)
        pb = None if B0 in self.vias else self.pad_at(B0, n, L)
        A, B = self.centred(moved.get(A0, A0), pa), self.centred(moved.get(B0, B0), pb)
        if not dirs or (A == A0 and B == B0 and len(dirs) == len(tracks)
                        and all(on_axis(p, q) for p, q in zip(pts, pts[1:]))):
            return None

        def end_slack(k, pad):
            if pad:
                return self.slack(pad)
            if k in slacks:
                return slacks[k]
            return None if k in moved else self.via_slack(k, n, tracks)
        sa, sb = end_slack(A0, pa), end_slack(B0, pb)
        res = self.solve(A, B, dirs, l0, sa, sb)
        if not res:
            return 'left'
        l, eA, eB = res
        A2, B2 = key((A[0] + eA[0], A[1] + eA[1])), key((B[0] + eB[0], B[1] + eB[1]))
        end = (A2[0] + sum(li * d[0] for li, d in zip(l, dirs)), A2[1] + sum(li * d[1] for li, d in zip(l, dirs)))
        if (math.hypot(end[0] - B2[0], end[1] - B2[1]) > 1e-3
                or min(l) < -1e-6 or max(abs(x - y) for x, y in zip(l, l0)) > SLIDE
                or math.hypot(*eA) > SLIDE or math.hypot(*eB) > SLIDE
                or (pa and not pa.HitTest(V(*A2))) or (pb and not pb.HitTest(V(*B2)))):
            return 'left'
        q, new = A2, [A2]
        for li, d in zip(l, dirs):
            if li >= 1e-4:
                q = (q[0] + li * d[0], q[1] + li * d[1])
                new.append(q)
        new[-1] = B2
        widths = [w for li, w in zip(l, wid) if li >= 1e-4]
        return (n, L, A0, B0, A, B, new, widths, tracks)

    def commit(self, plan):
        """replace a chain's tracks by its planned run, sliding a via end with it"""
        n, L, A0, B0, A, B, new, widths, tracks = plan
        locked = any(t.IsLocked() for t in tracks)
        for t in tracks:
            self.b.Delete(t)
        for p, q, w in zip(new, new[1:], widths):
            t = pcbnew.PCB_TRACK(self.b)
            t.SetStart(V(*p))
            t.SetEnd(V(*q))
            t.SetWidth(w)
            t.SetLayer(L)
            t.SetNetCode(n)
            t.SetLocked(locked)
            self.b.Add(t)
        segs = list(zip(new, new[1:]))
        for k, k2 in ((A0, new[0]), (B0, new[-1])):
            if k in self.vias:
                if k2 != k:
                    segs += self.slide(k, k2, n)
                self.touched |= {k, k2}
        return segs

    def plan_move(self, n, k, to, chains, keep=None):
        """where via k can go at `to`, or a few microns from it onto a run's axis, and the
        rebuilt runs that end on it; None if one does not fit"""
        at = [c for c in chains if c[0] == n and c[2] is not keep and k in (c[2][0], c[2][-1][1])]
        for _ in range(2):
            plans = [self.plan(*c, moved={k: to}) for c in at]
            stuck = [c for c, p in zip(at, plans) if p == 'left']
            if not stuck:
                return to, plans
            fits = []
            for d in DIRS[:4]:
                p = self.plan(*stuck[0], moved={k: to}, slacks={k: d})
                if p and p != 'left':
                    to2 = p[6][0] if k == stuck[0][2][0] else p[6][-1]
                    if dist(to2, to) <= 0.02:
                        fits.append((dist(to2, to), to2))
            if not fits:
                return None
            to = min(fits)[1]
        return None

    def move(self, n, k, to, plans):
        segs = []
        for p in plans:
            if p:
                segs += self.commit(p)
        segs += self.slide(k, to, n)
        self.touched |= {k, to}
        return segs

    def move_via(self, n, k, to, chains):
        """move via k aside to about `to`, rebuilding its runs; None if one does not fit"""
        m = self.plan_move(n, k, to, chains)
        return m and self.move(n, k, m[0], m[1])

    def shift(self, n, L, chain, chains):
        """a run with a via end that fits only if the via moves across its other tracks"""
        for k in (chain[0], chain[-1][1]):
            if k not in self.vias or k in self.touched or any(
                    p.GetNetCode() == n and p.HitTest(V(*k)) for p in self.pads):
                continue
            cands = []
            for d in DIRS[:4]:
                p = self.plan(n, L, chain, slacks={k: d})
                if p and p != 'left':
                    to = p[6][0] if k == chain[0] else p[6][-1]
                    if to != k:
                        cands.append((dist(k, to), to))
            for _, to in sorted(cands):
                # the via's other runs held at the new place, then this one
                m = self.plan_move(n, k, to, chains, keep=chain)
                if not m:
                    continue
                again = self.plan(n, L, chain, moved={k: m[0]})
                if again == 'left':
                    continue
                segs = self.move(n, k, m[0], m[1] + [again])
                return segs
        return None

    def straighten(self, keep=()):
        """returns {(A, B): [segments]} for the runs rebuilt, and the runs left as they were"""
        rebuilt, left = {}, []
        chains = self.chains()
        for c in chains:
            n, L, chain = c
            A, B = chain[0], chain[-1][1]
            if (A, B) in keep or (B, A) in keep or A in self.touched or B in self.touched:
                continue
            p = self.plan(n, L, chain)
            if p is None:
                continue
            name = chain[1][0].GetNetname()
            if p == 'left':
                segs = self.shift(n, L, chain, chains)
                if segs is None:
                    left.append((name, A, B))
                    continue
            else:
                segs = self.commit(p)
            rebuilt[(A, B)] = (name, segs)
        return rebuilt, left

    def drop_idle(self):
        """segments of no length, inside one pad, or lying on another track"""
        n = 0
        segs = self.segs()
        gone = set()
        for t in segs:
            a, c = P(t.GetStart()), P(t.GetEnd())
            p = self.pad_at(a, t.GetNetCode(), t.GetLayer())
            covered = any(u is not t and id(u) not in gone and u.GetNetCode() == t.GetNetCode()
                          and u.GetLayer() == t.GetLayer() and u.GetWidth() >= t.GetWidth()
                          and seg_dist(a, P(u.GetStart()), P(u.GetEnd())) < 1e-3
                          and seg_dist(c, P(u.GetStart()), P(u.GetEnd())) < 1e-3 for u in segs)
            if mm(t.GetLength()) < 0.002 or (p and p.HitTest(t.GetEnd())) or covered:
                gone.add(id(t))
                self.b.Delete(t)
                n += 1
        return n


def seg_dist(p, a, c):
    dx, dy = c[0] - a[0], c[1] - a[1]
    n = dx * dx + dy * dy
    t = 0 if n == 0 else max(0, min(1, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / n))
    return math.hypot(p[0] - a[0] - t * dx, p[1] - a[1] - t * dy)


def drc(path):
    """[(x, y)] of the tracks and vias in violations, the unconnected count, and the
    clearance violations as [(required, actual, [(kind, (x, y))])]"""
    rpt = path + '.straighten.rpt'
    subprocess.run(['kicad-cli', 'pcb', 'drc', '-o', rpt, path], capture_output=True)
    text = open(rpt).read()
    os.remove(rpt)
    pts = [(float(x), float(y)) for x, y in re.findall(r'@\(([\d.]+) mm, ([\d.]+) mm\): (?:Track|Via)', text)]
    unconnected = re.search(r'Found (\d+) unconnected', text)
    viols = []
    for block in re.split(r'\n(?=\[)', text):
        m = re.search(r'clearance ([\d.]+) mm; actual ([\d.]+) mm', block)
        if m:
            items = [(kind, (float(x), float(y))) for x, y, kind
                     in re.findall(r'@\(([\d.]+) mm, ([\d.]+) mm\): (\w+)', block)]
            viols.append((float(m.group(1)), float(m.group(2)), items))
    return pts, int(unconnected.group(1)) if unconnected else 0, viols


def off_axis(b):
    """[(a, c)] of the segments that run at an odd angle"""
    out = []
    for t in b.GetTracks():
        if t.Type() == pcbnew.PCB_TRACE_T and mm(t.GetLength()) >= 0.002:
            # the raw ends: rounding them to keys can tip a 45-degree run off by 1e-4
            a, c = P(t.GetStart()), P(t.GetEnd())
            if not on_axis(a, c):
                out.append((key(a), key(c)))
    return out


def apply(path, original, skip, nudges={}):
    """the three steps on the original board, leaving out the changes in skip and moving
    the vias in nudges; also returns the ends of segments the steps left at an odd angle"""
    open(path, 'wb').write(original)
    # load from the board's own directory: its .kicad_pro carries the rules SaveBoard keeps
    b = pcbnew.LoadBoard(path)
    odd = off_axis(b)
    changes = Straightener(b).snap(skip)
    changes.update(Straightener(b).joints(skip))
    st = Straightener(b)
    rebuilt, left = st.straighten(skip)
    changes.update(rebuilt)
    if nudges:
        st = Straightener(b)
        chains = st.chains()
        for k, to in nudges.items():
            if k in st.vias and k not in st.touched:
                n = st.vias[k].GetNetCode()
                segs = st.move_via(n, k, to, chains)
                if segs:
                    changes[('nudge', n, k)] = (st.vias[k].GetNetname(), segs)
    dropped = Straightener(b).drop_idle()
    # an odd angle the steps made, not one the board came with along the same line
    bent = [p for a, c in off_axis(b) if not any(seg_dist(a, a0, c0) < 0.05 and seg_dist(c, a0, c0) < 0.05
                                                 for a0, c0 in odd) for p in (a, c)]
    pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    pcbnew.SaveBoard(path, b)
    return changes, dropped, left, drc(path), bent


def nudges_for(change, segs, viols):
    """moves of another net's via, nearest first, that would clear the change's violations with it"""
    out = []
    for req, act, items in viols:
        mine = [p for _, p in items if any(seg_dist(p, a, e) < 0.01 for a, e in segs)]
        vias = [p for kind, p in items if kind == 'Via' and p not in mine]
        if len(mine) != 1 or len(vias) != 1:
            continue
        k = key(vias[0])
        m = min(((a, e) for a, e in segs), key=lambda s: seg_dist(k, *s))
        dx, dy = m[1][0] - m[0][0], m[1][1] - m[0][1]
        n = dx * dx + dy * dy
        u = 0 if n == 0 else max(0, min(1, ((k[0] - m[0][0]) * dx + (k[1] - m[0][1]) * dy) / n))
        away = (k[0] - m[0][0] - u * dx, k[1] - m[0][1] - u * dy)
        r = math.hypot(*away)
        if r < 1e-6:
            continue
        away = (away[0] / r, away[1] / r)
        for d in DIRS:
            dot = d[0] * away[0] + d[1] * away[1]
            if dot > 0.3:
                L = math.ceil((req - act + 0.002) / dot * 1e4) / 1e4
                if L <= SLIDE:
                    out.append((L, k, key((k[0] + L * d[0], k[1] + L * d[1]))))
    return [(k, to) for _, k, to in sorted(out)]


def one_pass(path, before, unconnected, rejected):
    """snap, joints and runs, undoing each change that breaks DRC or bends a track off
    axis, and moving another net's via aside where that clears a change; returns the report"""
    original = open(path, 'rb').read()

    def bad(pts):
        return [p for p in pts if all(dist(p, q) > 1e-3 for q in before)]

    skip, nudges = set(rejected), {}
    for _ in range(10):
        changes, dropped, left, (pts, unc, _), bent = apply(path, original, skip)
        broke = {c for c, (_, sg) in changes.items()
                 if any(seg_dist(p, a, e) < 0.01 for p in bad(pts) + bent for a, e in sg)}
        if unc > unconnected:
            broke |= {c for c in changes if c[0] == 'snap'}
        if not broke:
            break
        skip |= broke
    else:
        open(path, 'wb').write(original)
        sys.exit('straighten_tracks.py: DRC still fails, board left as it was')
    # a violation blames every change near it: give each blamed change a try of its own
    result, last = (changes, dropped, left), True
    for c in sorted(skip - set(rejected), key=str):
        changes, dropped, left, (pts, unc, viols), bent = apply(path, original, skip - {c}, nudges)
        last = not bad(pts) and not bent and unc <= unconnected
        if not last and not bent and unc <= unconnected and c in changes:
            for k, to in nudges_for(c, changes[c][1], viols):
                if k in nudges:
                    continue
                trial = apply(path, original, skip - {c}, {**nudges, k: to})
                changes, dropped, left, (pts, unc, viols), bent = trial
                last = not bad(pts) and not bent and unc <= unconnected
                if last:
                    nudges[k] = to
                    break
        if last:
            skip.discard(c)
            result = (changes, dropped, left)
        else:
            rejected[c] = 'leaves a track off axis' if bent else 'the change breaks DRC'
    if not last:
        result = apply(path, original, skip, nudges)[:3]
    return result


def main(path):
    path = os.path.abspath(path)
    start = open(path, 'rb').read()
    try:
        report(path)
    except BaseException:
        open(path, 'wb').write(start)
        raise
    sys.stdout.flush()
    os._exit(0)     # pcbnew can crash tearing down a modified board


def report(path):
    before, unconnected, _ = drc(path)
    total = defaultdict(int)
    rejected = {}
    for _ in range(6):
        changes, dropped, left = one_pass(path, before, unconnected, rejected)
        for c in changes:
            total[c[0] if isinstance(c[0], str) else 'run'] += 1
        total['idle'] += dropped
        if not changes and not dropped:
            break
    print('snapped %d ends, moved %d joints, rebuilt %d runs, moved %d vias aside, dropped %d idle segments'
          % (total['snap'], total['joint'], total['run'], total['nudge'], total['idle']))
    for name, A, B in left:
        print('  left %s %s-%s: no 45-degree fit' % (name, A, B))
    for c, why in sorted(rejected.items(), key=str):
        what = '%s %s' % (c[0], c[2]) if isinstance(c[0], str) else 'run %s-%s' % c
        print('  kept %s: %s' % (what, why))


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
