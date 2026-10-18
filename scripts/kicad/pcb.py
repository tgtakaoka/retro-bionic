"""Start a Bionic board from a schematic netlist, and manage its pours.

new_board() takes the stackup, design rules and outline of an existing
board (2 layers: MC68HC12B, 4 layers: Z380), loads every footprint from
the netlist, assigns nets, and places footprints from a table. Parts left
out of the table are lined up beside the board for placing by hand.

    pcb.py pours <board.kicad_pcb> strip|add [out.dsn]

strips the zones, or adds GND on the outer layers (and In1) with VCC on
In2 and refills them; with a DSN path it exports for freerouting instead
of saving.
"""
import os
import re
import sys

import pcbnew

from sexp import parse, find, uq, extract_block

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
BASE = {2: os.path.join(REPO, 'schematics/mc68hc12b/bionic-mc68hc12b.kicad_pcb'),
        4: os.path.join(REPO, 'schematics/z380/bionic-z380.kicad_pcb')}
STOCK = '/usr/share/kicad/footprints'


def read_netlist(path):
    """KiCad netlist (kicad-cli sch export netlist) -> (components, nets)"""
    tree = parse(open(path).read())[0]
    comps = {}
    for c in find(find(tree, 'components')[0], 'comp'):
        ref = uq(find(c, 'ref')[0][1])
        comps[ref] = dict(value=uq(find(c, 'value')[0][1]), fp=uq(find(c, 'footprint')[0][1]),
                          tstamp=uq(find(c, 'tstamps')[0][1]))
    nets = {}
    for n in find(find(tree, 'nets')[0], 'net'):
        name = uq(find(n, 'name')[0][1])
        nets[name] = [(uq(find(x, 'ref')[0][1]), uq(find(x, 'pin')[0][1])) for x in find(n, 'node')]
    return comps, nets


def _header(out, title, date, layers):
    """board file holding only the template's header and outline"""
    zt = open(BASE[layers]).read()
    et = open(BASE[4]).read()      # the Z380 outline is the common one
    head = zt[:zt.index('\n\t(net ')]
    head = head.replace(extract_block(head, '(pcbplotparams'), extract_block(open(BASE[2]).read(), '(pcbplotparams'))
    head = re.sub(r'\(title "[^"]*"\)', '(title "%s")' % title, head)
    head = re.sub(r'\(date "[^"]*"\)', '(date "%s")' % date, head)
    head = re.sub(r'\(rev "[^"]*"\)', '(rev "1")', head)
    edges = []
    pos = 0
    while True:
        i = et.find('\n\t(gr_', pos)
        if i < 0:
            break
        blk = extract_block(et[i + 2:], '(gr_')
        if '"Edge.Cuts"' in blk:
            edges.append('\t' + blk)
        pos = i + 2 + len(blk)
    with open(out, 'w') as f:
        f.write(head + '\n\t(net 0 "")\n' + '\n'.join(edges) + '\n)\n')


def zone(b, net, layers, prio=0):
    """a pour of net over the board outline"""
    bb = b.GetBoardEdgesBoundingBox()
    z = pcbnew.ZONE(b)
    ls = pcbnew.LSET()
    for layer in layers:
        ls.AddLayer(layer)
    z.SetLayerSet(ls)
    z.SetNet(net)
    o = z.Outline()
    o.NewOutline()
    for px, py in ((bb.GetLeft(), bb.GetTop()), (bb.GetRight(), bb.GetTop()),
                   (bb.GetRight(), bb.GetBottom()), (bb.GetLeft(), bb.GetBottom())):
        o.Append(px, py)
    z.SetMinThickness(pcbnew.FromMM(0.2))
    z.SetLocalClearance(pcbnew.FromMM(0.2))
    z.SetThermalReliefGap(pcbnew.FromMM(0.3))
    z.SetThermalReliefSpokeWidth(pcbnew.FromMM(0.3))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetAssignedPriority(prio)
    b.Add(z)
    return z


def _text(b, s, x, y, layer, size, thickness, back=False, bold=False):
    t = pcbnew.PCB_TEXT(b)
    t.SetText(s)
    t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I_MM(size, size))
    t.SetTextThickness(pcbnew.FromMM(thickness))
    t.SetBold(bold)
    if back:
        t.SetMirrored(True)
        t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_LEFT)
        t.SetVertJustify(pcbnew.GR_TEXT_V_ALIGN_BOTTOM)
    t.SetPosition(pcbnew.VECTOR2I_MM(x, y))
    b.Add(t)


def new_board(board_dir, name, title, date, netlist, placement, layers=2, sheet=None, outer_pours=False,
              title_silk=False):
    """placement: ref -> (x, y, rotation, on_back); title_silk puts the title on the front silkscreen.
    Returns the board path."""
    out = os.path.join(board_dir, name + '.kicad_pcb')
    _header(out, title, date, layers)
    b = pcbnew.LoadBoard(out)
    # revision on the back, and optionally the name along the bottom edge, as on MC68HC12B
    bb = b.GetBoardEdgesBoundingBox()
    cx, bottom, left = pcbnew.ToMM(bb.Centre().x), pcbnew.ToMM(bb.GetBottom()), pcbnew.ToMM(bb.GetLeft())
    if title_silk:
        _text(b, title, cx, bottom - 2.21, pcbnew.F_SilkS, 1.5, 0.3, bold=True)
    _text(b, 'Rev. 1', left + 6.07, bottom - 1.14, pcbnew.B_SilkS, 1, 0.1, back=True)
    comps, nets = read_netlist(netlist)
    netobj = {}
    for n in nets:
        ni = pcbnew.NETINFO_ITEM(b, n)
        b.Add(ni)
        netobj[n] = ni
    bb = b.GetBoardEdgesBoundingBox()
    spare_x, spare_y = pcbnew.ToMM(bb.GetRight()) + 10, pcbnew.ToMM(bb.GetTop())
    for ref, c in sorted(comps.items()):
        lib, fpname = c['fp'].split(':')
        local = os.path.join(board_dir, lib + '.pretty')
        fp = pcbnew.FootprintLoad(local if os.path.isdir(local) else os.path.join(STOCK, lib + '.pretty'), fpname)
        fp.SetFPID(pcbnew.LIB_ID(lib, fpname))
        fp.SetReference(ref)
        fp.SetValue(c['value'])
        fp.SetPath(pcbnew.KIID_PATH('/' + c['tstamp']))
        fp.SetSheetname('/')
        fp.SetSheetfile(sheet or name + '.kicad_sch')
        b.Add(fp)
        x, y, rot, back = placement.get(ref, (0, 0, 0, False))
        fp.SetPosition(pcbnew.VECTOR2I_MM(x, y))
        if back:
            fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_TOP_BOTTOM)
        fp.SetOrientationDegrees(rot)
        if ref not in placement:
            # stack the unplaced parts in a column right of the board
            box = fp.GetBoundingBox(False)
            fp.Move(pcbnew.VECTOR2I_MM(spare_x, spare_y) - box.GetOrigin())
            spare_y += pcbnew.ToMM(box.GetHeight()) + 1
    for n, nodes in nets.items():
        for ref, pin in nodes:
            for pad in b.FindFootprintByReference(ref).Pads():
                if pad.GetNumber() == pin:
                    pad.SetNet(netobj[n])
    if layers == 4:
        # inner layers carry signals under GND (In1) and VCC (In2) pours
        b.SetLayerType(pcbnew.In1_Cu, pcbnew.LT_SIGNAL)
        b.SetLayerType(pcbnew.In2_Cu, pcbnew.LT_SIGNAL)
        zone(b, netobj['GND'], [pcbnew.In1_Cu])
        zone(b, netobj['VCC'], [pcbnew.In2_Cu])
        if outer_pours:
            zone(b, netobj['GND'], [pcbnew.F_Cu, pcbnew.B_Cu])
    else:
        zone(b, netobj['GND'], [pcbnew.F_Cu, pcbnew.B_Cu])
    pcbnew.SaveBoard(out, b)
    if not title_silk:
        return out
    # the Python API cannot pick a font; name the title's face in the file
    with open(out) as f:
        text = f.read()
    i = text.index('(gr_text "%s"' % title)
    j = text.index('(font', i) + len('(font')
    with open(out, 'w') as f:
        f.write(text[:j] + '\n\t\t\t\t(face "Noto Mono")' + text[j:])
    return out


def pours(path, mode, dsn=None):
    b = pcbnew.LoadBoard(path)
    for z in list(b.Zones()):
        b.Delete(z)
    if mode == 'add':
        four = b.GetCopperLayerCount() == 4
        zone(b, b.FindNet('GND'), [pcbnew.F_Cu, pcbnew.B_Cu] + ([pcbnew.In1_Cu] if four else []))
        if four:
            zone(b, b.FindNet('VCC'), [pcbnew.In2_Cu])
        pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    if dsn:
        pcbnew.ExportSpecctraDSN(b, dsn)
    else:
        pcbnew.SaveBoard(path, b)


if __name__ == '__main__':
    if len(sys.argv) < 4 or sys.argv[1] != 'pours' or sys.argv[3] not in ('strip', 'add'):
        sys.exit(__doc__)
    pours(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else None)
