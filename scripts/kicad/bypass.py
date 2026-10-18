"""Make each supply pin take its current through its bypass capacitor.

Before routing, split() gives each supply pin a locked stub towards its
capacitor's pad on a private net VCC_<cap>_<pad>, stopping GAP short of
the pad, so the router feeds VCC to the capacitor pad and cannot reach
the pin another way. Tie pins (inputs strapped to VCC) go on private
nets TIE<n> of the Default class, so they route thin.

    bypass.py merge <board.kicad_pcb>

after routing closes each gap, folds the private nets back into VCC and
checks that every stubbed pin reaches VCC only through its capacitor pad.
"""
import math
import os
import sys

import pcbnew

GAP = 0.3
V = pcbnew.VECTOR2I_MM
mm = pcbnew.ToMM


def _pad(b, ref, num):
    return [p for p in b.FindFootprintByReference(ref).Pads() if p.GetNumber() == str(num)][0]


def _net(b, name):
    n = b.FindNet(name)
    if not n:
        n = pcbnew.NETINFO_ITEM(b, name)
        b.Add(n)
    return n


def _track(b, a, c, layer, width, net):
    t = pcbnew.PCB_TRACK(b)
    t.SetStart(V(*a))
    t.SetEnd(V(*c))
    t.SetWidth(pcbnew.FromMM(width))
    t.SetLayer(layer)
    t.SetNet(net)
    t.SetLocked(True)
    b.Add(t)


def split(b, stubs, ties=(), width=0.3, via=(0.45, 0.2)):
    """stubs: [(ic_ref, pin, cap_ref, cap_pad, waypoints)]; waypoints [(x, y)] lead from the pin
    to the capacitor pad, and a via at a waypoint given as (x, y, 'via') switches to the other side.
    ties: [(ic_ref, pin)]"""
    for ic, pin, cap, cpad, way in stubs:
        net = _net(b, 'VCC_%s_%s' % (cap, cpad))
        p = _pad(b, ic, pin)
        p.SetNet(net)
        c = _pad(b, cap, cpad)
        layer = pcbnew.F_Cu if p.IsOnLayer(pcbnew.F_Cu) else pcbnew.B_Cu
        pts = [(mm(p.GetPosition().x), mm(p.GetPosition().y))]
        for w in way:
            pts.append(w[:2])
            _track(b, pts[-2], pts[-1], layer, width, net)
            if len(w) > 2:
                v = pcbnew.PCB_VIA(b)
                v.SetPosition(V(*w[:2]))
                v.SetWidth(pcbnew.FromMM(via[0]))
                v.SetDrill(pcbnew.FromMM(via[1]))
                v.SetNet(net)
                v.SetLocked(True)
                b.Add(v)
                layer = pcbnew.B_Cu if layer == pcbnew.F_Cu else pcbnew.F_Cu
        # last leg towards the capacitor pad, stopping GAP short of its edge
        cx, cy = mm(c.GetPosition().x), mm(c.GetPosition().y)
        x, y = pts[-1]
        d = math.hypot(cx - x, cy - y)
        bb = c.GetBoundingBox()
        reach = abs(cx - x) / d * mm(bb.GetWidth()) / 2 + abs(cy - y) / d * mm(bb.GetHeight()) / 2
        k = max(0.0, (d - reach - GAP) / d)
        _track(b, (x, y), (round(x + (cx - x) * k, 4), round(y + (cy - y) * k, 4)), layer, width, net)
    for n, (ic, pin) in enumerate(ties):
        _pad(b, ic, pin).SetNet(_net(b, 'TIE%d' % (n + 1)))


def _items(b, net):
    code = net.GetNetCode()
    pads = [p for p in b.GetPads() if p.GetNetCode() == code]
    tracks = [t for t in b.GetTracks() if t.GetNetCode() == code]
    return pads, tracks


def merge(b, supply='VCC'):
    vcc = b.FindNet(supply)
    stubbed = []
    for name, net in list(b.GetNetsByName().items()):
        name = str(name)
        if name.startswith('VCC_'):
            cap, cpad = name[4:].rsplit('_', 1)
            c = _pad(b, cap, cpad)
            pads, tracks = _items(b, net)
            cp = c.GetPosition()
            # the stub end nearest the capacitor pad, on the pad's side
            ends = [(e, t.GetLayer()) for t in tracks if t.Type() == pcbnew.PCB_TRACE_T
                    for e in (t.GetStart(), t.GetEnd()) if c.IsOnLayer(t.GetLayer())]
            e, layer = min(ends, key=lambda el: (el[0] - cp).EuclideanNorm())
            _track(b, (mm(e.x), mm(e.y)), (mm(cp.x), mm(cp.y)), layer, mm(tracks[0].GetWidth()), vcc)
            for it in pads + tracks:
                it.SetNet(vcc)
            stubbed.append((pads[0], c))
        elif name.startswith('TIE'):
            pads, tracks = _items(b, net)
            for it in pads + tracks:
                it.SetNet(vcc)
    return stubbed


def check(b, stubbed, supply='VCC'):
    """every stubbed pin must lose VCC once its capacitor pad is taken away"""
    code = b.FindNet(supply).GetNetCode()
    items = [t for t in b.GetTracks() if t.GetNetCode() == code]
    pads = [p for p in b.GetPads() if p.GetNetCode() == code]
    bad = []
    for pin, cap in stubbed:
        others = [p for p in pads if p not in (pin, cap)]
        seen, todo = set(), [pin]
        reached = False
        while todo and not reached:
            cur = todo.pop()
            for t in items:
                if id(t) in seen or not _touches(t, cur):
                    continue
                seen.add(id(t))
                if any(_touches(t, p) for p in others):
                    reached = True
                    break
                todo.append(t)
        if reached:
            bad.append(pin.GetParentFootprint().GetReference() + '.' + pin.GetNumber())
    return bad


def _touches(t, item):
    """track or via t meets a pad, track or via (same layer for tracks)"""
    if isinstance(item, pcbnew.PAD):
        if t.Type() == pcbnew.PCB_VIA_T:
            return item.HitTest(t.GetPosition())
        return item.IsOnLayer(t.GetLayer()) and (item.HitTest(t.GetStart()) or item.HitTest(t.GetEnd()))
    if t.Type() == pcbnew.PCB_VIA_T and item.Type() == pcbnew.PCB_VIA_T:
        return False
    pts_t = [t.GetPosition()] if t.Type() == pcbnew.PCB_VIA_T else [t.GetStart(), t.GetEnd()]
    pts_i = [item.GetPosition()] if item.Type() == pcbnew.PCB_VIA_T else [item.GetStart(), item.GetEnd()]
    if t.Type() == pcbnew.PCB_TRACE_T and item.Type() == pcbnew.PCB_TRACE_T and t.GetLayer() != item.GetLayer():
        return False
    return any((a - c).EuclideanNorm() < 1000 for a in pts_t for c in pts_i)


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] != 'merge':
        sys.exit(__doc__)
    board = pcbnew.LoadBoard(sys.argv[2])
    st = merge(board)
    bad = check(board, st)
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    pcbnew.SaveBoard(sys.argv[2], board)
    print('merged %d stubs; %s' % (len(st), 'pins reached around their capacitor: ' + ' '.join(bad) if bad else 'all fed through their capacitors'))
    sys.stdout.flush()
    os._exit(0)
