"""INS8060 (SC/MP) plugin for scripts/record-cycles.py.

Each pattern is one opcode of ins8060.txt with HALT ($00), the debugger's
break, as its operand byte, run from ORG in memory filled with it, so any
transfer stops at once. A displacement of 0 puts every operand in plain
memory: the operand byte itself, or where a pointer points. Needs the
profile image (-D PROFILE_CYCLES), whose dump ends at the HALT's second
read, the one with the H flag, and prints the chip's own status flags (I,
D, H) on every cycle; the SC/MP marks its opcode fetches itself, so there
is no matcher to check, only the table: check() holds the cycles from the
pattern's fetch to the HALT's against its length and read and write
counts.

    P=debugger/ins8060/tools/cycles_ins8060.py
    R=debugger/ins8060/tools/ins8060-cycles.jsonl.zst
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

NAME = 'INS8060'
HALT = 0x00
DLY = 0x8F
# The PC is incremented before a fetch, within its 4K page, so ORG keeps
# clear of a page start.
ORG = 0x1100
ACIA = range(0xDF00, 0xDF10)
# Pointers point into the fill; S keeps interrupts disabled; A and E of 0
# give the shortest DLY.
# The PC is incremented before a fetch, so it points a byte before ORG.
BASE = [('PC', ORG - 1), ('P1', 0x2100), ('P2', 0x2200), ('P3', 0x2300),
        ('A', 0x00), ('E', 0x00), ('S', 0x00)]
FILL = [(0, ACIA.start, [HALT]), (ACIA.stop, 0x10000 - ACIA.stop, [HALT])]
CONDITIONAL = re.compile(r'^(JP|JZ|JNZ)$')    # on A


def fill_value(addr):
    return None if addr in ACIA else HALT


def table():
    """{opc: (mnemonic, operands, length, reads, writes)}; reads count the
    instruction's own bytes."""
    out = {}
    for line in open(os.path.join(ARCH, 'ins8060.txt')):
        f = line.split()
        if len(f) == 6 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[1] != '-':
            out[int(f[0], 16)] = (f[1], f[2], int(f[3]), int(f[4]), int(f[5]))
    return out


def patterns(args=None):
    out = []
    for opc, (mnemo, opr, length, reads, writes) in sorted(table().items()):
        if opc == HALT:
            continue
        base = dict(key='%02X' % opc, mnemo=mnemo, operands=opr,
                    bytes=[opc] + [HALT] * (length - 1))
        if CONDITIONAL.match(mnemo):
            # Zero and positive; then neither.
            out.append(dict(base, key=base['key'] + ':a00', regs={'A': 0x00}))
            out.append(dict(base, key=base['key'] + ':a9f', regs={'A': 0x9F}))
        else:
            out.append(base)
    return out


def cycles(txt):
    """[kind, addr, data, flags] per printed cycle; flags is a string of
    the status flags I, D and H that were set."""
    return [[m.group(1), int(m.group(2), 16), int(m.group(3), 16),
             m.group(4).replace('-', '')]
            for m in re.finditer(
                r'^[ IDH]([RW]) A=([0-9A-F]{4}) D=([0-9A-F]{2}) ([I-][D-][H-])$',
                txt, re.M)]


def record(run, cycles, ok):
    rec = dict(run, end='timeout' if not ok else 'other', cycles=cycles)
    if ok and len(cycles) >= 2 and 'H' in cycles[-1][3]:
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
    opc = int(rec['key'][:2], 16)
    mnemo, opr, length, reads, writes = table()[opc]
    head = '%-4s %-6s %d %dR %dW' % (mnemo, opr, length, reads, writes)
    trace = rec['cycles']
    if rec['end'] != 'trap':
        return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
    problems = []
    # The run ends with the HALT's fetch and its second read, the H one.
    body, trap = trace[:-2], trace[-2:]
    if (trap[0][1] != trap[1][1] or trap[0][2] != HALT or trap[1][2] != HALT
            or [c[0] for c in trap] != ['R', 'R']):
        problems.append('trap read %s' % ' '.join(
            '%s%04X:%02X' % (c[0], c[1], c[2]) for c in trap))
    if not body or body[0][1] != ORG or body[0][2] != opc:
        problems.append('first cycle not the fetch at %04X' % ORG)
    marks = [i for i, c in enumerate(trace) if 'I' in c[3]]
    if marks != [0, len(body)]:
        problems.append('I flag at %s' % ' '.join('%04X' % trace[i][1] for i in marks))
    delays = [i for i, c in enumerate(trace) if 'D' in c[3]]
    if delays != ([1] if opc == DLY else []):
        problems.append('D flag at %s' % ' '.join('%04X' % trace[i][1] for i in delays))
    halts = [i for i, c in enumerate(trace) if 'H' in c[3]]
    if halts != [len(trace) - 1]:
        problems.append('H flag at %s' % ' '.join('%04X' % trace[i][1] for i in halts))
    if [c[1] for c in body[:length]] != list(range(ORG, ORG + length)):
        problems.append('instruction bytes not read in order')
    # Table 2 of the data sheet: reads, the instruction's own first, then
    # the writes.
    kinds = ''.join(c[0] for c in body)
    if kinds != 'R' * reads + 'W' * writes:
        problems.append('table says %s, the chip %s' % ('R' * reads + 'W' * writes, kinds))
    return '%s %s' % (head, '; '.join(problems)) if problems else None
