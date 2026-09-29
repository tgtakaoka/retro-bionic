"""INS8070 plugin for scripts/record-cycles.py.

Each pattern is one opcode of ins8070.txt with CALL 15 ($1F), the
debugger's break, as its operand bytes, run from ORG in memory filled
with it, so any transfer stops at once. Needs the profile image
(-D PROFILE_CYCLES), whose dump marks the matcher's opcode fetches with L.
The loop drops the ring from the CALL 15's fetch on, so a run's cycles are
the pattern's own: check() holds them against the table's sequence and the
matcher's marks.

    P=debugger/ins8070/tools/cycles_ins8070.py
    R=debugger/ins8070/tools/ins8070-cycles.jsonl.zst
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

NAME = 'INS8070'
CALL15 = 0x1F
ORG = 0x1000
ACIA = range(0xDF00, 0xDF10)
INTERNAL = 0xFFC0               # on-chip RAM
# Pulls read the fill, pushes land in it, pointers point into it; S keeps
# interrupts disabled.
# The PC is incremented before a fetch, so it points a byte before ORG.
BASE = [('PC', ORG - 1), ('SP', 0x0F00), ('P2', 0x2000), ('P3', 0x2100),
        ('EA', 0x1F1F), ('T', 0x1F1F), ('S', 0x00)]
# A CALL 15 vector of 0000 is the debugger's stop, so every CALL 15 ends the run.
VEC_CALL15 = 0x003E
FILL = [(0, VEC_CALL15, [CALL15]), (VEC_CALL15, 2, [0x00]),
        (VEC_CALL15 + 2, ACIA.start - VEC_CALL15 - 2, [CALL15]),
        (ACIA.stop, INTERNAL - ACIA.stop, [CALL15])]
CONDITIONAL = re.compile(r'^(BZ|BNZ|BP|BND)$')    # on A


def fill_value(addr):
    if addr >= INTERNAL or addr in ACIA:
        return None
    return 0x00 if addr in (VEC_CALL15, VEC_CALL15 + 1) else CALL15


def table():
    """{opc: (mnemonic, operands, length, cycles, sequence)}; cycles has a
    letter per bus cycle, and alternatives after @ for pushes and pops that
    stay in on-chip RAM."""
    out = {}
    for line in open(os.path.join(ARCH, 'ins8070.txt')):
        f = line.split()
        if len(f) == 8 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[1] != '-':
            out[int(f[0], 16)] = (f[1], f[2], int(f[5]), f[6], f[7])
    return out


def patterns(args=None):
    out = []
    for opc, (mnemo, opr, length, cyc, seq) in sorted(table().items()):
        if opc == CALL15:
            continue
        base = dict(key='%02X' % opc, mnemo=mnemo, operands=opr,
                    bytes=[opc] + [CALL15] * (length - 1))
        if CONDITIONAL.match(mnemo):
            # Zero, positive and a digit; then none of them.
            out.append(dict(base, key=base['key'] + ':a00', regs={'A': 0x00}))
            out.append(dict(base, key=base['key'] + ':a9f', regs={'A': 0x9F}))
        else:
            out.append(base)
    return out


def cycles(txt):
    """[kind, addr, data, fetch] per printed cycle."""
    return [[m.group(1), int(m.group(2), 16), int(m.group(3), 16), bool(m.group(4))]
            for m in re.finditer(r'^([RW]) A=([0-9A-F]{4}) D=([0-9A-F]{2})( L)?$',
                                 txt, re.M)]


def record(run, cycles, ok):
    rec = dict(run, end='timeout' if not ok else 'other', cycles=cycles)
    if ok and cycles and cycles[0][1] == ORG:
        rec['end'] = 'trap'
    return rec


def restore(run, cycles):
    out = []
    for addr in sorted({c[1] for c in cycles if c[0] == 'W'}):
        value = fill_value(addr)
        if value is not None:
            out.append((addr, [value]))
    return out


def check(rec):
    mnemo, opr, length, cyc, seq = table()[int(rec['key'][:2], 16)]
    head = '%-5s %-9s %-14s' % (mnemo, opr, seq)
    trace = rec['cycles']
    if rec['end'] != 'trap':
        return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
    # Each alternative ends with the next instruction's fetch.
    want = {len(c) - 1 for c in cyc.split('@')}
    problems = []
    marks = [i for i, c in enumerate(trace) if c[3]]
    if marks != [0]:
        problems.append('matcher marked fetches at %s' % ' '.join(
            '%04X' % trace[i][1] for i in marks))
    if len(trace) not in want:
        problems.append('table says %s cycles, the chip %d'
                        % ('/'.join(map(str, sorted(want))), len(trace)))
    return '%s %s' % (head, '; '.join(problems)) if problems else None
