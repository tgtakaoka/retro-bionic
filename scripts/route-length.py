#!/usr/bin/env python3
"""Compare routed track length with the straight-line distance between pads.

Summary (default): every routed net, ranked by how much copper it uses
beyond the shortest straight-line tree joining its pads (a minimum
spanning tree; for a two-pad net, the pad-to-pad distance).

    scripts/route-length.py schematics/mc68hc16z/bionic-mc68hc16z.kicad_pcb

Per pad (--net): the copper path from a source pad to each other pad of
the net against the straight line between them. The source is the
net's connector pad (a J reference) unless --source names one.

    scripts/route-length.py board.kicad_pcb --net ASEL0 --net ASEL1

Reads the saved file, so save the board in KiCad first. Copper pours are
not counted; nets carried by them (GND, VCC by default) are skipped.
Needs KiCad's Python module (pcbnew): run with the python KiCad ships.
"""

import argparse
import collections
import fnmatch
import heapq
import math
import sys

import pcbnew

mm = pcbnew.ToMM


def point(p):
    return (p.x, p.y)


class Net:
    """The copper of one net as a graph: nodes are points in internal
    units, edges carry their length in mm."""

    def __init__(self, board, name):
        self.name = name
        self.tracks = []
        self.vias = []
        for t in board.GetTracks():
            if t.GetNetname() != name:
                continue
            (self.vias if t.Type() == pcbnew.PCB_VIA_T else self.tracks).append(t)
        self.pads = {}
        for fp in board.GetFootprints():
            for p in fp.Pads():
                if p.GetNetname() == name:
                    self.pads['%s.%s' % (fp.GetReference(), p.GetNumber())] = p
        self.graph = collections.defaultdict(list)
        self._build()

    def length(self):
        return sum(mm(t.GetLength()) for t in self.tracks)

    def _join(self, a, b, length=0.0):
        self.graph[a].append((b, length))
        self.graph[b].append((a, length))

    def _build(self):
        ends = [(point(t.GetStart()), t) for t in self.tracks] + \
               [(point(t.GetEnd()), t) for t in self.tracks]
        for t in self.tracks:
            s, e = point(t.GetStart()), point(t.GetEnd())
            if t.Type() == pcbnew.PCB_ARC_T:
                self._join(s, e, mm(t.GetLength()))
                continue
            # A track may end anywhere along a straight one (a T): split
            # the straight one there.
            on = {s, e}
            for p, o in ends:
                if o is not t and o.GetLayer() == t.GetLayer() and \
                        t.HitTest(pcbnew.VECTOR2I(*p), 0):
                    on.add(p)
            dx, dy = e[0] - s[0], e[1] - s[1]
            pts = sorted(on, key=lambda p: (p[0] - s[0]) * dx + (p[1] - s[1]) * dy)
            for a, b in zip(pts, pts[1:]):
                self._join(a, b, mm(int(math.dist(a, b))))
        nodes = list(self.graph)
        for v in self.vias:
            c = point(v.GetPosition())
            for n in nodes:
                if v.HitTest(pcbnew.VECTOR2I(*n)):
                    self._join(c, n)
        for p in self.pads.values():
            c = point(p.GetPosition())
            for (n, t) in ends:
                if p.IsOnLayer(t.GetLayer()) and p.HitTest(pcbnew.VECTOR2I(*n)):
                    self._join(c, n)

    def paths_from(self, pad):
        """Copper path length in mm from |pad| to every reachable node."""
        start = point(self.pads[pad].GetPosition())
        dist = {start: 0.0}
        queue = [(0.0, start)]
        while queue:
            d, u = heapq.heappop(queue)
            if d > dist[u]:
                continue
            for v, l in self.graph[u]:
                if d + l < dist.get(v, math.inf):
                    dist[v] = d + l
                    heapq.heappush(queue, (d + l, v))
        return dist

    def pad_point(self, pad):
        return point(self.pads[pad].GetPosition())

    def straight(self, a, b):
        return mm(int(math.dist(self.pad_point(a), self.pad_point(b))))

    def spanning(self):
        """Length of the minimum spanning tree over the pads, in mm."""
        names = list(self.pads)
        if len(names) < 2:
            return 0.0
        best = {n: self.straight(names[0], n) for n in names[1:]}
        total = 0.0
        while best:
            n = min(best, key=best.get)
            total += best.pop(n)
            for m in best:
                best[m] = min(best[m], self.straight(n, m))
        return total


def summary(board, args):
    rows = []
    names = sorted({t.GetNetname() for t in board.GetTracks()} - {''})
    for name in names:
        if any(fnmatch.fnmatchcase(name, x) for x in args.exclude):
            continue
        net = Net(board, name)
        if len(net.pads) < 2:
            continue
        route, direct = net.length(), net.spanning()
        rows.append((name, route, direct, len(net.vias), len(net.pads)))
    key = (lambda r: r[1] / r[2] if r[2] else 0) if args.sort == 'ratio' \
        else (lambda r: r[1] - r[2])
    rows.sort(key=key, reverse=True)
    print('%-16s %8s %8s %8s %6s %5s %5s' %
          ('net', 'route', 'direct', 'excess', 'ratio', 'vias', 'pads'))
    for name, route, direct, vias, pads in rows[:args.top or None]:
        print('%-16s %8.2f %8.2f %8.2f %6.2f %5d %5d' %
              (name, route, direct, route - direct,
               route / direct if direct else 0, vias, pads))


def per_pad(board, args):
    for pattern in args.net:
        names = sorted(n for n in {t.GetNetname() for t in board.GetTracks()}
                       if fnmatch.fnmatchcase(n, pattern)) or [pattern]
        for name in names:
            net = Net(board, name)
            if not net.pads:
                print('%s: no such net' % name, file=sys.stderr)
                continue
            source = args.source if args.source in net.pads else \
                next((p for p in sorted(net.pads) if p.startswith('J')),
                     sorted(net.pads)[0])
            dist = net.paths_from(source)
            print('%s: %.2f mm routed, %d vias, from %s' %
                  (name, net.length(), len(net.vias), source))
            print('  %-10s %8s %8s %8s %6s' %
                  ('pad', 'route', 'direct', 'excess', 'ratio'))
            others = [p for p in net.pads if p != source]
            others.sort(key=lambda p: dist.get(net.pad_point(p), math.inf))
            for pad in others:
                route = dist.get(net.pad_point(pad))
                direct = net.straight(source, pad)
                if route is None:
                    print('  %-10s %8s %8.2f   not connected' % (pad, '-', direct))
                else:
                    print('  %-10s %8.2f %8.2f %8.2f %6.2f' %
                          (pad, route, direct, route - direct,
                           route / direct if direct else 0))


def main():
    parser = argparse.ArgumentParser(
        description=__doc__.split('\n\n')[0],
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('board', help='.kicad_pcb file')
    parser.add_argument('--net', action='append', default=[],
                        help='show each pad of this net (glob; repeatable)')
    parser.add_argument('--source', help='pad the paths start from, e.g. J2.29')
    parser.add_argument('--top', type=int, default=10,
                        help='nets in the summary (0 for all)')
    parser.add_argument('--sort', choices=('excess', 'ratio'), default='excess')
    parser.add_argument('--exclude', action='append', default=['GND', 'VCC'],
                        help='nets left out of the summary (glob; repeatable)')
    args = parser.parse_args()
    board = pcbnew.LoadBoard(args.board)
    if args.net:
        per_pad(board, args)
    else:
        summary(board, args)


if __name__ == '__main__':
    main()
