"""TMS7000 family plugin for scripts/record-cycles.py.

Each pattern is one opcode of tms7000.txt, run from ORG in memory filled
with TRAP 23 ($E8), so any transfer stops at once. Needs the profile image
(-D PROFILE_CYCLES), whose run stops when a TRAP 23 reads its vector and
whose dump prints every cycle, the matcher's fetches as F and the cycles it
gives each as m=. Only external and peripheral-file cycles reach the bus;
register-file accesses, pushes and pops among them, never do. check() holds
the matcher's fetches against the run: the pattern's opcode, then the
TRAP's, which is the cycle before the vector read.

IDLE never reaches the TRAP: a CMOS part fetches it, perhaps reads once
more, then drives no bus cycle until an interrupt. Each wait for ALATCH
that gives up prints "?halt: no bus cycle" and leaves a cycle that isn't
one; the run ends 'idle', and the reset after it wakes the CPU.

    P=debugger/tms7000/tools/cycles_tms7000.py
    R=debugger/tms7000/tools/tms7000-cycles.jsonl.zst
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

NAME = ('TMS7000', 'TMS7000NL4', 'TMS70C00', 'TMS7001', 'TMS7001NL4', 'TMS7002',
        'TMS70C02')
TRAP23 = 0xE8
VEC_TRAP23 = (0xFFD0, 0xFFD1)
IDLE = 0x01
ORG = 0x1000
EXTERNAL = 0x0200               # past the register and peripheral files
# Operands: a register, a pointer register pair and a peripheral, all clear
# of A, B, the stack and the I/O ports; the stack holds a return into the
# fill, and ST keeps interrupts disabled. B is the index of @a16(B).
REG = 0x40
PTR = 0x5D                      # R5C:R5D
SP = 0x60
PORT = 0x80                     # P128, >0180: external on every chip
BASE = [('PC', ORG), ('SP', SP), ('A', 0x00), ('B', 0x00), ('ST', 0x00)]
# The pointer pair, then what POP ST, RETS and RETI pull: ST, PC MSB, PC LSB.
SEED = (PTR - 1, [TRAP23, TRAP23, 0x00, TRAP23, TRAP23])
assert SEED[0] + len(SEED[1]) - 1 == SP
FILL = [(EXTERNAL, 0x10000 - EXTERNAL, [TRAP23])]
PAD = 4                         # the longest instruction


def fill_value(addr):
    return TRAP23 if addr >= EXTERNAL else None


def table():
    """{opc: (mnemonic, operands, length, bus cycles)}; the matcher counts
    length + bus cycles per instruction."""
    out = {}
    for line in open(os.path.join(ARCH, 'tms7000.txt')):
        f = line.split()
        if len(f) == 5 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[1] != '-':
            out[int(f[0], 16)] = (f[1], f[2], int(f[3]),
                                  0 if f[4] == '-' else int(f[4]))
    return out


# Operand bytes, in the order they follow the opcode.
OPERAND = {'Rs': [REG], 'Rd': [REG], 'Rn': [REG], 'Rp': [REG],
           '*Rp': [PTR], 'Ps': [PORT], 'Pd': [PORT], 'Pn': [PORT],
           '%n': [TRAP23], '%n16': [TRAP23, TRAP23], '@a16': [TRAP23, TRAP23],
           '@a16(B)': [TRAP23, TRAP23], 'r8': [TRAP23]}


def patterns(args=None):
    out = []
    for opc, (mnemo, opr, length, bus) in sorted(table().items()):
        if opc == TRAP23:
            continue
        code = [opc]
        for o in opr.split(','):
            code += OPERAND.get(o, [])
        assert len(code) == length, '%02X %s %s' % (opc, mnemo, opr)
        base = dict(key='%02X' % opc, mnemo=mnemo, operands=opr,
                    bytes=code + [TRAP23] * (PAD - len(code)))
        if '*Rp' in opr or mnemo in ('RETS', 'RETI') or opc == 0x08:
            base['data_seed'] = SEED
        out.append(base)
    return out


STALL = '?halt: no bus cycle'


def cycles(txt):
    """[kind, addr, data, fetch, m] per printed cycle; kind is R or W, or
    r or w for a peripheral-file cycle inside the chip. A cycle after a wait
    for ALATCH that gave up gets a ? after its kind: every later wait gives
    up too, and the last one has no cycle after it."""
    out = [['R' if m.group(1) == 'F' else m.group(1), int(m.group(2), 16),
            int(m.group(3), 16), m.group(1) == 'F', int(m.group(4))]
           for m in re.finditer(
               r'^([FRWrwI]) A=([0-9A-F]{4}) D=([0-9A-F]{2}) m=(\d+)$', txt, re.M)]
    stalls = txt.count(STALL)
    if stalls:
        # the first cycle, the pattern's opcode fetch, is always a real one
        for c in out[max(1, len(out) - stalls + 1):]:
            c[0] += '?'
    return out


def stalled(cycles):
    """The index of the first cycle that isn't one, or None."""
    return next((i for i, c in enumerate(cycles) if c[0].endswith('?')), None)


def trap_fetch(cycles):
    """The index of the TRAP 23's fetch: the cycle before its vector read."""
    for i, c in enumerate(cycles):
        if c[0] == 'R' and c[1] in VEC_TRAP23:
            return i - 1
    return None


def record(run, cycles, ok):
    rec = dict(run, end='timeout' if not ok else 'other', cycles=cycles)
    t = trap_fetch(cycles)
    if ok and t is not None and t > 0 and cycles[t][2] == TRAP23:
        rec['end'] = 'trap'
    elif ok and run['mnemo'] == 'IDLE' and stalled(cycles):
        rec['end'] = 'idle'
    return rec


def restore(run, cycles):
    out = []
    for addr in sorted({c[1] for c in cycles if c[0].rstrip('?') == 'W'}):
        value = fill_value(addr)
        if value is not None:
            out.append((addr, [value]))
    return out


def check(rec):
    mnemo, opr, length, bus = table()[int(rec['key'][:2], 16)]
    head = '%-5s %-10s %d+%d' % (mnemo, opr, length, bus)
    trace = rec['cycles']
    if mnemo == 'IDLE':
        return check_idle(head, trace, rec['end'])
    if rec['end'] != 'trap':
        return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
    t = trap_fetch(trace)
    problems = []
    if length + bus != t:
        problems.append('table says %d cycles, the chip %d' % (length + bus, t))
    marks = [i for i, c in enumerate(trace) if c[3]]
    if marks != [0, t]:
        problems.append('matcher marked fetches at %s' % ' '.join(
            '%04X' % trace[i][1] for i in marks))
    return '%s %s' % (head, '; '.join(problems)) if problems else None


def check_idle(head, trace, end):
    if end == 'idle':
        n = stalled(trace)
        if n > 2 or not trace[0][3] or trace[0][1:3] != [ORG, IDLE]:
            return '%s idled after %d cycles: %s' % (head, n, ' '.join(
                '%s %04X' % (c[0], c[1]) for c in trace[:n]))
        return None
    refetch = sum(1 for c in trace[1:] if c[1:3] == [ORG, IDLE])
    if refetch:
        return '%s fetched itself %d more times instead of idling (an NMOS part?)' % (
            head, refetch)
    return '%s ended %s after %d cycles without idling' % (head, end, len(trace))
