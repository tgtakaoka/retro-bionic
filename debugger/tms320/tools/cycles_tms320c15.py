"""TMS320C15 plugin for scripts/record-cycles.py.

Each pattern is one opcode word of inst_tms3201x-P00.txt or, for 7Fxx, of
inst_tms3201x-P7F.txt, run from ORG in program memory filled with OUT 0
(>4800): only OUT strobes #WE at a port address, so the profile image
(-D PROFILE_CYCLES) ends a run at the first such write after the
pattern's own first instruction, and any transfer stops at once. Its dump
prints every bus cycle, P for a program read, and I with m=<n> where the
matcher took an opcode fetch of n cycles: check() holds both against the
chip. Every cycle reaches the bus: each machine cycle reads program
memory, except the #DEN and #WE cycles of IN, OUT and TBLW. The profile
run keeps IN and OUT off the devices and TBLW off memory, so every port
is run and nothing needs restoring.

    P=debugger/tms320/tools/cycles_tms320c15.py
    R=debugger/tms320/tools/tms320c15-cycles.jsonl.zst
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

NAME = 'TMS320C15'
TRAP = 0x4800                   # OUT 0,>00
ORG = 0x100
TARGET = 0x800                  # branch addresses and ACC, in the fill
FILL = [(0x000, 0x1000, [TRAP])]
# Direct operands address page 0 of the data RAM, clear of the debugger's
# working word (>0F). ACC is TBLR's and TBLW's program address, CALA's
# target, and positive; at 8 or above, so a TBLW is never taken for the
# trap's OUT. AR0 makes BANZ branch; ST clears OV, ARP and DP.
DMA = 0x10
POS, NEG, ZERO = TARGET, 0xFFFFF000 | TARGET, 0
BASE = [('PC', ORG), ('ST', 0x0000), ('ACC', POS), ('AR0', DMA), ('AR1', DMA),
        ('BIO', 1)]
# LST loads ST from DMA: keep BASE's.
LST_SEED = (DMA, [0x0000])

# The ACC values for each way a conditional branch on ACC goes.
ACC_BRANCH = {'BLZ': (NEG, POS), 'BLEZ': (ZERO, POS), 'BGZ': (POS, ZERO),
              'BGEZ': (ZERO, NEG), 'BNZ': (POS, ZERO), 'BZ': (ZERO, POS)}


def _rows(name):
    for line in open(os.path.join(ARCH, 'inst_tms3201x-%s.txt' % name)):
        f = line.split()
        if f and re.fullmatch(r'[0-9A-F][0-9A-Fx]', f[0]):
            yield f


def table():
    """{opcode word: (mnemonic, cycles)}, one word per row the matcher
    tells apart: the high byte, and for 7Fxx the low byte."""
    out = {}
    for f in _rows('P00'):
        if f[1] == '-' or f[5] == 'PAGE7F':
            continue
        if f[0].endswith('x'):
            high = int(f[0][0], 16) << 4
            msbs = [high | n for n in range(16)
                    if all(b == 'x' or int(b) == (n >> (3 - i)) & 1
                           for i, b in enumerate(f[2]))]
        else:
            msbs = [int(f[0], 16)]
        for msb in msbs:
            out[msb << 8] = (f[5], int(f[6]))
    for f in _rows('P7F'):
        if f[1] != '-':
            out[0x7F00 | int(f[0], 16)] = (f[5], int(f[6]))
    return out


def _word(opc, mnemo):
    """The pattern's first word: the opcode with its operand byte."""
    if opc >= 0xF400 or opc == 0x6E00 or opc >> 8 == 0x7F:
        return opc                      # branch, LDPK 0, page 7F
    return opc | DMA                    # direct operand, or K


def _variants(mnemo):
    """(suffix, taken, regs) for each way a branch goes."""
    if mnemo in ACC_BRANCH:
        t, n = ACC_BRANCH[mnemo]
        return [(':t', True, {'ACC': t}), (':n', False, {'ACC': n})]
    if mnemo == 'BANZ':
        return [(':t', True, {}), (':n', False, {'AR0': 0})]
    if mnemo == 'BV':
        return [(':t', True, {'ST': 0x8000}), (':n', False, {})]
    if mnemo == 'BIOZ':
        # The BIO pseudo-register drives #BIO low.
        return [(':t', True, {'BIO': 0}), (':n', False, {})]
    return [('', True, {})]


def patterns(args=None):
    for opc, (mnemo, cyc) in sorted(table().items()):
        word = _word(opc, mnemo)
        words = [word, TARGET] if opc >= 0xF400 else [word]
        for suffix, taken, regs in _variants(mnemo):
            run = dict(key='%04X%s' % (word, suffix), mnemo=mnemo,
                       bytes=words, taken=taken)
            if regs:
                run['regs'] = regs
            if mnemo == 'LST':
                run['data_seed'] = LST_SEED
            yield run


def cycles(txt):
    """[kind, space, addr, data, matched] per printed cycle; kind is I for
    the matcher's opcode fetches, P for other program reads, R and W for
    #DEN and #WE; space is I for a port address."""
    return [[m.group(1), m.group(2), int(m.group(3), 16), int(m.group(4), 16),
             int(m.group(5))]
            for m in re.finditer(
                r'^([IPRW]) ([AI])=([0-9A-F]{3}) D=([0-9A-F]{4}) m=(\d+)$', txt, re.M)]


def record(run, cycles, ok):
    """Every cycle, up to the trap's port write; 'trap' is the index of
    its opcode fetch, the cycle before."""
    rec = dict(run, end='timeout' if not ok else 'other', cycles=cycles)
    if (ok and len(cycles) > 2 and cycles[-1][:2] == ['W', 'I']
            and cycles[-2][0] in 'IP' and cycles[-2][3] == TRAP):
        rec.update(end='trap', trap=len(cycles) - 2)
    return rec


def span(cycles):
    """The debug pin spans every printed cycle, up to the trap's port
    write; an opcode fetch is a program read on the bus, and the channels
    carry D0-D7 only."""
    return [('P' if c[0] in 'IP' else c[0], c[3] & 0xFF) for c in cycles]


def bus_cycles(rows, col):
    """A cycle per #MEN, #DEN or #WE rising after it was low for at least
    50ns, with D0-D7 as it rises. restore() leaves the first fetch's #MEN
    low before the debug pin rises."""
    d = [col('D%d' % i) for i in range(8)]
    strobes = [(col('#MEN'), 'P'), (col('#DEN'), 'R'), (col('#WE'), 'W')]
    # A strobe low as the span opens began before it: count its rise.
    fall = {c: float('-inf') for c, _ in strobes if rows and rows[0][c] == '0'}
    out, prev = [], None
    for row in rows:
        if prev is not None:
            for c, kind in strobes:
                if prev[c] == '1' and row[c] == '0':
                    fall[c] = float(row[0])
                elif prev[c] == '0' and row[c] == '1' and c in fall:
                    if float(row[0]) - fall.pop(c) >= 50e-9:
                        out.append((kind, sum(int(prev[a]) << i for i, a in enumerate(d))))
        prev = row
    return out


def restore(run, cycles):
    """Nothing: the profile run keeps TBLW's write off memory."""
    return []


def _target(rec):
    """Where the pattern should hand over to the trap, None where the
    hardware stack decides."""
    mnemo = rec['mnemo']
    if mnemo == 'RET':
        return None
    if mnemo == 'CALA':
        return POS & 0xFFF
    if len(rec['bytes']) == 2 and rec.get('taken', True):
        return TARGET
    return ORG + len(rec['bytes'])


_TABLE = None


def check(rec):
    """The recording against the .txt tables and the matcher's marks."""
    global _TABLE
    _TABLE = _TABLE or table()
    word = rec['bytes'][0]
    opc = word & 0xFF00 if word >> 8 != 0x7F else word
    mnemo, cyc = _TABLE[opc]
    head = '%-5s %04X' % (mnemo, word)
    trace = rec['cycles']
    if rec['end'] != 'trap':
        return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
    took = rec['trap']
    problems = []
    if took != cyc:
        problems.append('table says %d cycles, the chip %d' % (cyc, took))
    marks = [i for i, c in enumerate(trace) if c[0] == 'I']
    if marks != [0, took] or trace[0][4] != took:
        problems.append('matcher marked %s, m=%d; the chip fetched at 0 and %d'
                        % (marks, trace[0][4], took))
    trap_cyc = _TABLE[TRAP & 0xFF00][1]
    if trace[took][4] != trap_cyc or len(trace) - took != trap_cyc:
        problems.append('trap took %d cycles, m=%d' % (len(trace) - took, trace[took][4]))
    want = _target(rec)
    if want is not None and trace[took][2] != want:
        problems.append('trap fetched at %03X, not %03X' % (trace[took][2], want))
    return '%s %s' % (head, '; '.join(problems)) if problems else None
