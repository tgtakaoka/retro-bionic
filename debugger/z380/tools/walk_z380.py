#!/usr/bin/env python3
"""The Z380 bus walker on the host, from the z380-PAGExx.txt tables: the
reference for inst_z380.cpp, and its check against the bench.

The Z380 fetches code as a stream of aligned words (a byte at an odd
target, or for a 7-byte instruction's last two) up to 5 bytes ahead of
what it runs. A walk follows that stream from an instruction start:
each instruction's bytes are fetched, then its data cycles and, when it
transfers control, the fetch at its target come in any order, while the
stream goes on ahead. An interrupt, NMI or trap is the PC pushed and a
vector fetched, in either order, between two instructions. The ring's
end must leave the CPU at the stop PC, which may be an instruction the
queue already holds.

    walk_z380.py records        every recorded run, as one instruction
    walk_z380.py rings [-v]     the bench rings: every instruction walked
                                must start a line of the sample's listing
    walk_z380.py fixture        test/z380/test_inst_z380/walks.inc: the
                                rings and a share of the runs, each with
                                what this walk made of it
"""
import json
import os
import re
import sys
from compression import zstd   # Python 3.14+

HERE = os.path.dirname(os.path.abspath(__file__))
Z380 = os.path.dirname(HERE)
PROJ = os.path.dirname(os.path.dirname(Z380))
PAGES = ['00', 'CB', 'ED', 'EDCB', 'DD', 'FD', 'DDCB', 'FDCB']
KINDS = 'RWrw'
ORG = 0x100
# DDIR decoder directives: the immediate bytes they add, the word size
# they force.
DDIR = {(0xDD, 0xC0): (0, 'W'), (0xDD, 0xC1): (1, 'W'), (0xDD, 0xC2): (2, 'W'),
        (0xDD, 0xC3): (1, None), (0xFD, 0xC0): (0, 'LW'), (0xFD, 0xC1): (1, 'LW'),
        (0xFD, 0xC2): (2, 'LW'), (0xFD, 0xC3): (2, None)}
VECTORS = (0x0000, 0x0038, 0x0066)   # trap, mode 1, NMI


class Fail(Exception):
    pass


# ------------------------------------------------------------ tables
def counts(text):
    """'R2W2' -> (2, 2, 0, 0); None for '-'."""
    if text == '-':
        return None
    n = dict(re.findall(r'([RWrw])(\d+)', text))
    return tuple(int(n.get(k, 0)) for k in KINDS)


def load_tables():
    tab = {}
    for page in PAGES:
        for line in open(os.path.join(Z380, 'z380-PAGE%s.txt' % page)):
            f = line.split()
            if len(f) < 2 or not re.match(r'^[0-9A-F]{2}$', f[0]):
                continue
            if f[1] == '-':
                continue
            opc, mnemo, operands, ln, c, x, data, nott, lw, xm = f[:10]
            row = dict(mnemo=mnemo, operands=operands, len=int(ln), cls=c.upper(),
                       cond=c.islower(), ext=x == '+')
            if row['cls'] == 'B':
                row['block'] = [(k, None if s == '.' else int(s))
                                for k, s in re.findall(r'([RWrw])([+-]\d|\.)', data)]
            else:
                row['data'] = counts(data)
                row['not'] = counts(nott)
                row['lw'] = counts(lw)
                row['xm'] = counts(xm)
            tab[(page, int(opc, 16))] = row
    return tab


# ------------------------------------------------------------ decoding
class Decoder:
    def __init__(self, tab, mem):
        self.tab, self.mem = tab, mem

    def decode(self, a):
        m = self.mem
        b0, b1 = m(a), m(a + 1)
        ddir = DDIR.get((b0, b1))
        base = a + 2 if ddir else a
        ext = ddir[0] if ddir else 0
        if ddir:
            b0, b1 = m(base), m(base + 1)
        if b0 == 0xCB:
            key, oplen = ('CB', b1), 2
        elif b0 == 0xED:
            key, oplen = (('EDCB', m(base + 2)) if b1 == 0xCB else ('ED', b1)), 2
        elif b0 in (0xDD, 0xFD) and b1 == 0xCB:
            key, oplen = ('%02XCB' % b0, m(base + 3 + ext)), 2
        elif b0 in (0xDD, 0xFD):
            key, oplen = ('%02X' % b0, b1), 2
        else:
            key, oplen = ('00', b0), 1
        row = self.tab.get(key)
        if row is None:
            return None
        if ddir and not row['ext'] and row.get('lw') is None:
            # one the directive does not affect: two bytes of its own
            return dict(addr=a, len=2, row=DDIR_ROW, ddir=None, ext=0, oplen=2, base=a)
        if ddir and not row['ext']:
            ext = 0
        return dict(addr=a, len=(base - a) + row['len'] + ext, row=row, ddir=ddir,
                    ext=ext, oplen=oplen, base=base)

    def operand(self, ins, n):
        """The instruction's last |n| bytes, little endian."""
        end = ins['addr'] + ins['len']
        return sum(self.mem(end - n + i) << (8 * i) for i in range(n))

    def target(self, ins, xm):
        row = ins['row']
        mask = 0xFFFFFFFF if xm else 0xFFFF
        if row['cls'] == 'A':
            return self.operand(ins, 2 + ins['ext']) & mask
        if row['cls'] == 'L':
            n = ins['len'] - (ins['base'] - ins['addr']) - ins['oplen']
            v = self.operand(ins, n)
            if v & (1 << (8 * n - 1)):
                v -= 1 << (8 * n)
            return (ins['addr'] + ins['len'] + v) & mask
        if row['cls'] == 'T':
            return self.mem(ins['base']) & 0x38
        if row['cls'] == 'X':
            return 0
        return None

    def needs(self, ins, mode, taken):
        row = ins['row']
        if row['cond'] and not taken:
            return row['not']
        word = ins['ddir'][1] if ins['ddir'] else None
        lw = word == 'LW' or (word is None and mode == 'lw')
        n = row['data']
        if lw and row['lw'] is not None:
            n = row['lw']
        if mode == 'xm' and row['xm'] is not None:
            n = tuple(a + x - b for a, x, b in zip(n, row['xm'], row['data']))
        return n


DDIR_ROW = dict(mnemo='DDIR', operands='', len=2, cls='-', cond=False, ext=False,
                data=(0, 0, 0, 0), lw=None, xm=None)
INTR_ROW = dict(mnemo='(interrupt)', operands='', len=0, cls='-', cond=False)


# ------------------------------------------------------------ walking
def nbytes(c):
    return 2 if c[4] == 1 else 1


def is_fetch(c, fetch):
    return c[0] == 'R' and c[1] == fetch


def advance(c):
    return (c[1] & ~1) + 2 if c[4] == 1 else c[1] + 1


def swap(d):
    return int(d[2:4], 16) << 8 | int(d[0:2], 16)


def walk(cyc, dec, mode, start, addr, stop):
    """[(instruction, taken, its data cycle indexes)] from cycle |start|, a
    fetch holding the instruction at |addr|, to the ring's end."""
    return _walk(cyc, dec, mode, start + 1, addr, advance(cyc[start]), [], stop)


def _end(out, pc, stop):
    """The CPU stops at |stop|: there, or an instruction the queue held,
    the ones decoded after it not run."""
    if pc == stop:
        return out
    for n in range(len(out) - 1, -1, -1):
        ins, taken, mine = out[n]
        if mine or taken:
            break
        if ins['addr'] == stop:
            return out[:n]
    raise Fail('ring ends at %X, stop at %X' % (pc, stop))


def _interrupt(cyc, mode, i, pc, fetch):
    """The PC pushed and a vector fetched, in either order, after an
    acknowledge or not: (next i, vector, fetch, pushed) or None."""
    j = i
    while j < len(cyc) and is_fetch(cyc[j], fetch):
        fetch = advance(cyc[j])
        j += 1
    if j >= len(cyc):
        return None
    c = cyc[j]
    target = None
    if c[0] == 'a':
        v = int(c[2].strip(' .')[-2:], 16)
        if v & 0xC7 == 0xC7:
            target = v & 0x38          # mode 0: a RST
        j += 1
    elif c[0] != 'W':
        return None
    left = 4 if mode == 'xm' else 2
    pushed = []
    vector = None
    while j < len(cyc) and (left > 0 or vector is None):
        c = cyc[j]
        if c[0] == 'W' and left >= nbytes(c):
            pushed.append(j)
            left -= nbytes(c)
        elif c[0] == 'R' and vector is None and (
                c[1] == target if target is not None else c[1] in VECTORS):
            vector = j
            fetch = advance(c)
        elif is_fetch(c, fetch):
            fetch = advance(c)
        else:
            return None
        j += 1
    if vector is None or left > 0:
        return None
    # but for a trap, the PC pushed is the one interrupted; None: any
    if pc is not None and cyc[vector][1] != 0 and len(pushed) == 1 and \
            swap(cyc[pushed[0]][2]) != pc & 0xFFFF:
        return None
    return j, cyc[vector][1], fetch, pushed


# Data cycles the first instruction may leave to one before the ring.
LEAD_MAX = 4


def _lead(out, mine, lead, c):
    """Whether |c| can be an earlier instruction's, ahead of the first."""
    return not out and not mine and lead < LEAD_MAX and c[0] in KINDS


def _walk(cyc, dec, mode, i, pc, fetch, out, stop):
    while True:
        intr = _interrupt(cyc, mode, i, pc, fetch)
        if intr is not None:
            i, pc, fetch, pushed = intr
            out = out + [(dict(addr=pc, len=0, row=INTR_ROW), True, pushed)]
        ins = dec.decode(pc)
        if ins is None:
            raise Fail('undecodable at %X' % pc)
        lead = 0
        while fetch < pc + ins['len']:
            if i >= len(cyc):
                return _end(out + [(ins, None, [])], pc, stop)
            c = cyc[i]
            if not is_fetch(c, fetch) and _lead(out, [], lead, c):
                lead += 1
                i += 1
                continue
            if not is_fetch(c, fetch):
                raise Fail('fetch %X expected, got %s%X at %d' % (fetch, c[0], c[1], i))
            fetch = advance(c)
            i += 1
        row = ins['row']
        if row['cls'] == 'B':
            mine = []
            i, fetch = _block(cyc, i, fetch, row['block'], mine)
            out = out + [(ins, False, mine)]
            pc += ins['len']
            if i >= len(cyc):
                return _end(out, pc, stop)
            continue
        if row['cond']:
            options = [True, False]
        else:
            options = [row['cls'] in ('A', 'L', 'I', 'T', 'X')]
        last = None
        for taken in options:
            try:
                return _execute(cyc, dec, mode, i, pc, fetch, ins, taken, out, stop)
            except Fail as e:
                last = e
        raise last


def _execute(cyc, dec, mode, i, pc, fetch, ins, taken, out, stop):
    """Its data cycles and, if |taken|, its target's fetch, in any order;
    the sequential fetches go on meanwhile."""
    need = list(dec.needs(ins, mode, taken))
    # a word split by its alignment is consecutive bytes: an instruction's
    # memory reads, and its writes, are contiguous
    after = {}
    target = dec.target(ins, mode == 'xm') if taken else None
    pending = taken
    nextpc = pc + ins['len']
    mine = []
    lead = 0
    while any(need) or pending:
        if i >= len(cyc):
            out = out + [(ins, taken, mine)]
            return _end(out, pc, stop) if pc == stop else \
                _end(out, target if pending and target is not None else nextpc, stop)
        c = cyc[i]
        k = KINDS.find(c[0])
        # an interrupt taken before the target's fetch pushes the target
        if pending and not any(need) and c[0] in 'aW':
            intr = _interrupt(cyc, mode, i, target, fetch)
            if intr is not None:
                j, vector, fetch2, pushed = intr
                out = out + [(ins, taken, mine),
                             (dict(addr=vector, len=0, row=INTR_ROW), True, pushed)]
                if j >= len(cyc):
                    return _end(out, vector, stop)
                return _walk(cyc, dec, mode, j, vector, fetch2, out, stop)
        # a target the queue was about to fetch anyway is still the target
        if pending and c[0] == 'R' and c[1] == target:
            pending = False
            nextpc = c[1]
            fetch = advance(c)
        elif is_fetch(c, fetch):
            fetch = advance(c)
        elif k >= 0 and need[k] >= nbytes(c) and after.get(c[0], c[1]) == c[1]:
            need[k] -= nbytes(c)
            if c[0] in 'RW':
                after[c[0]] = advance(c)
            mine.append(i)
        elif pending and target is None and c[0] == 'R':   # indirect
            pending = False
            nextpc = c[1]
            fetch = advance(c)
        elif _lead(out, mine, lead, c):
            lead += 1
        else:
            raise Fail('%s%X not wanted by %s at %d' % (c[0], c[1], ins['row']['mnemo'], i))
        i += 1
    out = out + [(ins, taken, mine)]
    # a pipeline flush fetches the next instruction, or the target, again
    if i < len(cyc) and cyc[i][0] == 'R' and \
            cyc[i][1] in (nextpc, nextpc & ~1) and cyc[i][1] != fetch:
        fetch = advance(cyc[i])
        i += 1
    if i >= len(cyc):
        return _end(out, nextpc, stop)
    return _walk(cyc, dec, mode, i, nextpc, fetch, out, stop)


def _block(cyc, i, fetch, pattern, mine):
    """Iterations while each transfer steps from its previous one."""
    last = [None] * len(pattern)
    j = 0
    while i < len(cyc):
        c = cyc[i]
        if is_fetch(c, fetch):
            fetch = advance(c)
            i += 1
            continue
        kind, step = pattern[j]
        if c[0] != kind or (last[j] is not None and step is not None
                            and c[1] != last[j] + step):
            break
        last[j] = c[1]
        mine.append(i)
        i += 1
        j = (j + 1) % len(pattern)
    return i, fetch


def trim(out):
    """A ring that begins inside an instruction can be walked from a byte
    of it: drop the instructions ahead of the first that moved data,
    transferred or was an interrupt, which nothing confirms."""
    for n, (ins, taken, mine) in enumerate(out):
        if mine or taken:
            return out[n:]
    return out


def find_start(cyc, dec, mode, stop):
    """The earliest fetch whose walk ends at |stop|: (index, addr, walk)."""
    last = None
    for i, c in enumerate(cyc):
        if c[0] != 'R':
            continue
        for a in [c[1]] + ([c[1] + 1] if c[4] == 1 else []):
            try:
                return i, a, trim(walk(cyc, dec, mode, i, a, stop))
            except Fail as e:
                last = e
    raise Fail(str(last))


# ------------------------------------------------------------ checks
def memory_of(cyc, base=None):
    """What the reads showed, over |base|: a breakpoint's RST 38H wins."""
    mem = dict(base or {})
    for c in cyc:
        if c[0] != 'R':
            continue
        a, d = c[1] & 0xFFFFFF, c[2]
        if c[4] == 1:
            mem[a & ~1], mem[a | 1] = int(d[0:2], 16), int(d[2:4], 16)
        else:
            mem[a] = int(d[2:4] if a & 1 else d[0:2], 16)
    return lambda a: mem.get(a & 0xFFFFFF, 0xFF)


def check_records(tab):
    recs = [json.loads(l) for l in zstd.open(os.path.join(HERE, 'z380-profile.jsonl.zst'), 'rt')]
    bad = 0
    for r in recs:
        if r['end'] != 'exit' or r.get('anomaly'):
            continue
        cyc = r['cycles']
        code = {ORG + i: b for i, b in enumerate(bytes.fromhex(r['bytes']))}
        dec = Decoder(tab, memory_of(cyc, code))
        mode = 'xm' if r['key'].endswith(':xm') else 'lw' if r['key'].endswith(':lw') else 'n'
        try:
            out = walk(cyc, dec, mode, 0, ORG, r['exit_pushed'] - 1)
            first = out[0][0]
            # a trap is taken before the instruction it is for
            trap = first['row'] is INTR_ROW and first['addr'] == 0
            why = None if first['addr'] == ORG or trap else 'first at %X' % first['addr']
        except Fail as e:
            why = str(e)
        if why:
            bad += 1
            print('%-22s %-6s %-20s %s' % (r['key'], r['mnemo'], r['operands'], why))
    print('%d of %d runs not walked' % (bad, len(recs)))


def listing(name, cap=None):
    """Where the sample's instructions start: as the ring was captured, if
    it says, else as the listing is now."""
    if cap and 'starts' in cap:
        return set(cap['starts'])
    starts = set()
    for line in open(os.path.join(PROJ, 'samples', 'z380', name + '.lst')):
        m = re.match(r'^(?:\(\d+\))?\s+([0-9A-F]+) : [0-9A-F]{2}', line)
        if m:
            starts.add(int(m.group(1), 16))
    return starts


def hex_memory(name, cap=None):
    if cap and 'image' in cap:
        return {int(a): b for a, b in cap['image'].items()}
    mem = {}
    for line in open(os.path.join(PROJ, 'samples', 'z380', name + '.hex')):
        line = line.strip()
        if line.startswith(':'):
            b = bytes.fromhex(line[1:])
            if b[3] == 0:
                for i in range(b[0]):
                    mem[(b[1] << 8 | b[2]) + i] = b[4 + i]
    return mem


def check_rings(tab, verbose):
    sys.path.insert(0, HERE)
    import cycles_z380
    caps = json.load(zstd.open(os.path.join(HERE, 'z380-rings.json.zst'), 'rt'))
    bad = 0
    for cap in caps:
        cyc = cycles_z380.cycles(cap['text'])
        stop = int(re.search(r'PC=([0-9A-F]+)', cap['text']).group(1), 16)
        dec = Decoder(tab, memory_of(cyc, hex_memory(cap['sample'], cap)))
        try:
            i, a, out = find_start(cyc, dec, 'n', stop)
        except Fail as e:
            bad += 1
            print('%-10s %2d  FAIL %s' % (cap['sample'], cap['halt'], e))
            continue
        starts = listing(cap['sample'], cap)
        off = [o[0]['addr'] for o in out if o[0]['row'] is not INTR_ROW
               and o[0]['addr'] not in starts]
        bad += bool(off)
        print('%-10s %2d  from %d/%d, %d instructions, stop %X%s' % (
            cap['sample'], cap['halt'], i, len(cyc), len(out), stop,
            '  NOT IN LISTING ' + ' '.join('%X' % x for x in off) if off else ''))
        if verbose:
            for ins, taken, mine in out:
                print('    %06X %-12s %s %s' % (ins['addr'], ins['row']['mnemo'],
                      'T' if taken else ' ', ' '.join('%s%X' % (cyc[j][0], cyc[j][1]) for j in mine)))
    print('%d of %d rings failed' % (bad, len(caps)))


# ------------------------------------------------------------ fixture
def fixture_case(name, cyc, mem, mode, stop, out, start):
    """One walk as C++: its cycles, the memory it decodes, the expected."""
    # the bytes any instruction start could decode: around the fetches
    near = set()
    for c in cyc:
        if c[0] == 'R':
            near.update(range((c[1] & 0xFFFFFF) - 8, (c[1] & 0xFFFFFF) + 10))
    mem = {a: b for a, b in mem.items() if a in near}
    lines = ['    {"%s", %s, %s, 0x%X, %d,' % (name, 'true' if mode == 'xm' else 'false',
                                             'true' if mode == 'lw' else 'false', stop, start)]
    # kind, address, =data; . for a byte transfer
    lines.append('        "' + ' '.join('%s%X=%s%s' % (
        c[0], c[1], c[2].replace('.', '0').replace(' ', '0'),
        '' if c[4] == 1 else '.') for c in cyc) + '",')
    addrs = sorted(mem)
    runs, i = [], 0
    while i < len(addrs):   # contiguous byte runs
        j = i
        while j + 1 < len(addrs) and addrs[j + 1] == addrs[j] + 1:
            j += 1
        runs.append((addrs[i], bytes(mem[a] for a in addrs[i:j + 1]).hex().upper()))
        i = j + 1
    lines.append('        "' + ' '.join('%X:%s' % r for r in runs) + '",')
    # the instructions walked; ! an interrupt taken
    lines.append('        "' + ' '.join('%s%X' % ('!' if o[0]['row'] is INTR_ROW else '',
                                                  o[0]['addr']) for o in out) + '"},')
    return '\n'.join(lines)


def fixture(tab):
    sys.path.insert(0, HERE)
    import cycles_z380
    cases = []
    for cap in json.load(zstd.open(os.path.join(HERE, 'z380-rings.json.zst'), 'rt')):
        cyc = cycles_z380.cycles(cap['text'])
        stop = int(re.search(r'PC=([0-9A-F]+)', cap['text']).group(1), 16)
        base = hex_memory(cap['sample'], cap)
        mem = {}
        memory_of(cyc, base)   # what the reads showed wins, as on the board
        for c in cyc:
            if c[0] == 'R':
                a, d = c[1] & 0xFFFFFF, c[2]
                if c[4] == 1:
                    base[a & ~1], base[a | 1] = int(d[0:2], 16), int(d[2:4], 16)
                else:
                    base[a] = int(d[2:4] if a & 1 else d[0:2], 16)
        i, a, out = find_start(cyc, Decoder(tab, lambda x: base.get(x & 0xFFFFFF, 0xFF)),
                               'n', stop)
        cases.append(fixture_case('%s-%d' % (cap['sample'], cap['halt']), cyc, base, 'n',
                                  stop, out, i))
    recs = [json.loads(l) for l in zstd.open(os.path.join(HERE, 'z380-profile.jsonl.zst'), 'rt')]
    for n, r in enumerate(recs):
        if r['end'] != 'exit' or r.get('anomaly'):
            continue
        row = tab.get((r['page'].split('/')[-1], r['opc']))
        interesting = '/' in r['page'] or r['key'].endswith((':xm', ':lw')) or (
            row is not None and row['cls'] != '-')
        if n % (5 if interesting else 23):
            continue
        cyc = r['cycles']
        mem = {ORG + i: b for i, b in enumerate(bytes.fromhex(r['bytes']))}
        for c in cyc:
            if c[0] == 'R':
                a, d = c[1] & 0xFFFFFF, c[2]
                if c[4] == 1:
                    mem.setdefault(a & ~1, int(d[0:2], 16))
                    mem.setdefault(a | 1, int(d[2:4], 16))
                else:
                    mem.setdefault(a, int(d[2:4] if a & 1 else d[0:2], 16))
        mode = 'xm' if r['key'].endswith(':xm') else 'lw' if r['key'].endswith(':lw') else 'n'
        dec = Decoder(tab, lambda x, m=mem: m.get(x & 0xFFFFFF, 0xFF))
        try:
            i, a, out = find_start(cyc, dec, mode, r['exit_pushed'] - 1)
        except Fail:
            continue
        cases.append(fixture_case(r['key'], cyc, mem, mode, r['exit_pushed'] - 1, out, i))
    path = os.path.join(PROJ, 'test', 'z380', 'test_inst_z380', 'walks.inc')
    with open(path, 'w') as f:
        f.write('// Generated by debugger/z380/tools/walk_z380.py fixture: the bench\n'
                '// rings and a share of the recorded runs, each with its walk.\n'
                '// clang-format off\n')
        f.write('\n'.join(cases) + '\n// clang-format on\n')
    print('wrote %s: %d walks' % (path, len(cases)))


def main():
    what = sys.argv[1] if len(sys.argv) > 1 else 'rings'
    tab = load_tables()
    if what == 'fixture':
        fixture(tab)
    elif what == 'records':
        check_records(tab)
    else:
        check_rings(tab, '-v' in sys.argv)


if __name__ == '__main__':
    main()
