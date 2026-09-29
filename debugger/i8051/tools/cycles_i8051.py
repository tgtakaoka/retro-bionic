"""P8051/P80C51 plugin for scripts/record-cycles.py.

Each pattern is one opcode of i8051.txt with MOVX @DPTR,A ($F0) as its
operand bytes, run from ORG in program memory filled with it. The 8051
has no trap instruction, so the profile image (-D PROFILE_CYCLES) ends a
run at the first external write after the pattern's own instruction:
any transfer lands in the fill and stops at once. Its dump prints every
bus cycle, P for a program read, and I with m=<n> where the matcher
took an opcode fetch of n bus cycles: check() holds both against the
chip. Record each chip on its own:

    P=debugger/i8051/tools/cycles_i8051.py
    R=debugger/i8051/tools/p8051-cycles.jsonl.zst    # or p80c51-
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

NAME = ('P8051', 'P80C51')
TRAP = 0xF0                     # MOVX @DPTR,A
ORG = 0x1000
# All program memory is external (EA low), and the 8051 cannot write it.
FILL = [(0x0000, 0x10000, [TRAP])]
# The trap writes A to external data at DPTR, plain RAM (the USART is at
# FFF0); MOVX @Ri reaches FF00|Ri, P2 being FF since reset. Direct and bit
# operands ($F0) are B and B.0; @Ri point into the scratch-pad RAM above
# the register banks, and the stack sits above them.
SP = 0x60
DPTR = 0x2000
BASE = [('PC', ORG), ('PSW', 0x00), ('SP', SP), ('DPTR', DPTR), ('A', TRAP),
        ('B', 0x02), ('R0', 0x40), ('R1', 0x41)]
# RET and RETI pop the trap's address from the internal RAM under SP.
RET_SEED = (SP - 1, [TRAP, TRAP])

TRANSFERS = {'AJMP', 'LJMP', 'SJMP', 'JMP', 'ACALL', 'LCALL', 'RET', 'RETI',
             'JC', 'JNC', 'JZ', 'JNZ', 'JB', 'JNB', 'JBC', 'CJNE', 'DJNZ'}


def table():
    """{opcode: (mnemonic, operands, length, bus cycles)}."""
    out = {}
    for line in open(os.path.join(ARCH, 'i8051.txt')):
        f = line.split()
        if len(f) == 5 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[1] != '-':
            out[int(f[0], 16)] = (f[1], f[2], int(f[3]), int(f[4]))
    return out


def _variants(opc, mnemo, opr):
    """(suffix, taken, regs, data_seed) for each way a conditional goes."""
    reg = 'R%d' % (opc & 7)
    if mnemo in ('JC', 'JNC'):
        on, off = {'PSW': 0x80}, {'PSW': 0x00}
    elif mnemo in ('JZ', 'JNZ'):
        on, off = {'A': 0x00}, {'A': TRAP}
    elif mnemo in ('JB', 'JNB', 'JBC'):
        on, off = {'B': 0x01}, {'B': 0x02}                  # B.0
    elif mnemo == 'DJNZ':
        r = 'B' if opr.startswith('a8') else reg
        return [(':t', True, {r: 0x02}, None), (':n', False, {r: 0x01}, None)]
    elif mnemo == 'CJNE':
        if opr.startswith('A,#'):
            ne, eq = {'A': 0x00}, {'A': TRAP}
        elif opr.startswith('A,a8'):
            ne, eq = {'B': 0x02}, {'B': TRAP}
        elif opr.startswith('@'):
            at = 0x40 + (opc & 1)                           # R0/R1 of BASE
            return [(':t', True, {}, (at, [0x00])), (':n', False, {}, (at, [TRAP]))]
        else:
            ne, eq = {reg: 0x00}, {reg: TRAP}
        return [(':t', True, ne, None), (':n', False, eq, None)]
    else:
        return [('', True, {}, None)]
    if mnemo in ('JNC', 'JNZ', 'JNB'):
        on, off = off, on
    return [(':t', True, on, None), (':n', False, off, None)]


def patterns(args=None):
    for opc, (mnemo, opr, length, bus) in sorted(table().items()):
        for suffix, taken, regs, seed in _variants(opc, mnemo, opr):
            run = dict(key='%02X%s' % (opc, suffix), mnemo=mnemo, operands=opr,
                       bytes=[opc] + [TRAP] * (length - 1), taken=taken)
            if regs:
                run['regs'] = regs
            if mnemo in ('RET', 'RETI'):
                seed = RET_SEED
            if seed:
                run['data_seed'] = seed
            yield run


def cycles(txt):
    """[kind, addr, data, matched] per printed cycle; kind is I for the
    matcher's opcode fetches, P for other program reads."""
    return [[m.group(1), int(m.group(2), 16), int(m.group(3), 16), int(m.group(4))]
            for m in re.finditer(r'^([IPRW ]) A=([0-9A-F]{4}) D=([0-9A-F]{2}) m=(\d+)',
                                 txt, re.M)]


def record(run, cycles, ok):
    """Every cycle, up to the trap's write; 'trap' is the index of its
    opcode fetch, two cycles before it."""
    end = 'timeout' if not ok else 'other'
    rec = dict(run, end=end, cycles=cycles)
    if ok and len(cycles) > 3 and cycles[-1][0] == 'W' and cycles[-3][0] in 'IP':
        rec.update(end='trap', trap=len(cycles) - 3)
    return rec


def span(cycles):
    """The debug pin spans every printed cycle, up to the trap's write;
    an opcode fetch is a program read on the bus."""
    return [('P' if c[0] in 'IP' else c[0], c[2]) for c in cycles]


def bus_cycles(rows, col):
    """A cycle per #PSEN, #RD or #WR rising after it was low for at least
    50ns, with AD0-AD7 as it rises."""
    ad = [col('AD%d' % i) for i in range(8)]
    strobes = [(col('#PSEN'), 'P'), (col('#RD'), 'R'), (col('#WR'), 'W')]
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
                        out.append((kind, sum(int(prev[a]) << i for i, a in enumerate(ad))))
        prev = row
    return out


def restore(run, cycles):
    """Nothing: the chip cannot write program memory, and what it writes
    to external data memory no pattern relies on."""
    return []


def _target(rec, length):
    """Where the pattern's last instruction should hand over to the trap."""
    mnemo, opc = rec['mnemo'], int(rec['key'][:2], 16)
    nxt = ORG + length
    if mnemo not in TRANSFERS or not rec.get('taken', True):
        return nxt
    if mnemo in ('LJMP', 'LCALL', 'RET', 'RETI'):
        return TRAP << 8 | TRAP
    if mnemo in ('AJMP', 'ACALL'):
        return (nxt & 0xF800) | (opc & 0xE0) << 3 | TRAP
    if mnemo == 'JMP':                                      # @A+DPTR
        return DPTR + TRAP
    return (nxt + TRAP - 0x100) & 0xFFFF                    # rel


_TABLE = None


def check(rec):
    """The recording against i8051.txt and the matcher's marks."""
    global _TABLE
    _TABLE = _TABLE or table()
    opc = int(rec['key'][:2], 16)
    mnemo, opr, length, bus = _TABLE[opc]
    head = '%-5s %-11s' % (mnemo, opr)
    trace = rec['cycles']
    if rec['end'] != 'trap':
        return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
    if len(rec['bytes']) != length:
        return '%s recorded with %d bytes' % (head, len(rec['bytes']))
    took = rec['trap']
    problems = []
    if took != bus:
        problems.append('table says %d cycles, the chip %d' % (bus, took))
    marks = [i for i, c in enumerate(trace) if c[0] == 'I']
    if marks != [0, took] or trace[0][3] != took:
        problems.append('matcher marked %s, m=%d; the chip fetched at 0 and %d'
                        % (marks, trace[0][3], took))
    want = _target(rec, length)
    if trace[took][1] != want:
        problems.append('trap fetched at %04X, not %04X' % (trace[took][1], want))
    return '%s %s' % (head, '; '.join(problems)) if problems else None
