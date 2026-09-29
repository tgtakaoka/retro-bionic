"""F3850 (Fairchild F8) plugin for scripts/record-cycles.py.

Each pattern is one opcode of f3850.txt with NOP ($2B) as its operand
bytes, run from ORG in memory filled with NOP. The F8 has no trap: every
instruction ends with the ROMC 00 fetch of the next, so the profile image
(-D PROFILE_CYCLES) stops at the run's second fetch, which takes a fill NOP
wherever the pattern went and so leaves the CPU at an instruction boundary.
Its dump shows each cycle's ROMC state, its length in XTLY periods (x=), and
on a fetch the cycles the table gives that instruction (m=): check() holds
them against the table and against the ROMC sequences of the F3850 data
sheet's Table 3, Instruction Cycle Execution and Timing.

    P=debugger/f3850/tools/cycles_f3850.py
    R=debugger/f3850/tools/f3850-cycles.jsonl.zst
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

NAME = 'F3850'
NOP = 0x2B
ORG = 0x1000
# The memory address registers live in the emulated memory, which has no
# I/O or internal RAM: all of it is plain. Data pointers and the jump
# registers point into the fill. A of 0 keeps OUTS 1 from pulling port 1's
# pins low against the debugger, which drives them high; W of 0 keeps
# interrupts disabled.
BASE = [('PC', ORG), ('DC', 0x2000), ('DC1', 0x2100), ('H', 0x2200),
        ('Q', 0x3000), ('K', 0x3100), ('P', 0x3200), ('IS', 0o20),
        ('W', 0x00), ('A', 0x00)]
FILL = [(0, 0x10000, [NOP])]
PAD = 4      # every write is this long, so no longer earlier pattern lingers
# x= of a short and a long cycle: 4 and 6 phi, as the chip measures them.
XTLY = {'S': 4, 'L': 6}


def table():
    """{opc: (mnemonic, operands, length, cycles)} for the opcodes the
    matcher takes as instructions."""
    out = {}
    for line in open(os.path.join(ARCH, 'f3850.txt')):
        f = line.split()
        if len(f) == 5 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and int(f[3]):
            out[int(f[0], 16)] = (f[1], f[2], int(f[3]), int(f[4]))
    return out


def manual(opc):
    """The cycles after |opc|'s fetch up to the next fetch, from Table 3, as
    a tuple of alternatives, each a list of (length, ROMC); None where the
    data sheet does not list the opcode."""
    def seq(text):
        return [(t[0], int(t[1:], 16)) for t in text.split()]
    if opc == 0x08:
        return (seq('L07 L0B S00'),)
    if opc == 0x09:
        return (seq('L15 L18 S00'),)
    if opc == 0x0C:
        return (seq('L12 L14 S00'),)
    if opc == 0x0D:
        return (seq('L17 L14 S00'),)
    if opc in (0x0E, 0x11):
        return (seq('L06 L09 S00'),)
    if opc in (0x0F, 0x10):
        return (seq('L16 L19 S00'),)
    if opc == 0x16:
        return (seq('L02 S00'),)
    if opc == 0x17:
        return (seq('L05 S00'),)
    if opc in (0x1A, 0x1B, 0x1D, 0xA0, 0xA1, 0xB0, 0xB1) or 0xD0 <= opc <= 0xDE:
        return (seq('S1C S00'),)
    if opc == 0x1C:
        return (seq('S04 S00'),)
    if 0x20 <= opc <= 0x25:
        return (seq('L03 S00'),)
    if opc == 0x26:
        return (seq('L03 L1B S00'),)
    if opc == 0x27:
        return (seq('L03 L1A S00'),)
    if opc == 0x28:
        return (seq('L03 S0D L0C L14 S00'),)
    if opc == 0x29:
        return (seq('L03 L0C L14 S00'),)
    if opc == 0x2A:
        return (seq('L11 S03 L0E S03 S00'),)
    if opc == 0x2C:
        return (seq('S1D S00'),)
    if 0x30 <= opc <= 0x3E:
        return (seq('L00'),)
    if 0x80 <= opc <= 0x87 or 0x90 <= opc <= 0x9F:
        return (seq('S1C L01 S00'), seq('S1C S03 S00'))    # taken, not
    if 0x88 <= opc <= 0x8D:
        return (seq('L02 S00'),)
    if opc == 0x8E:
        return (seq('L0A S00'),)
    if opc == 0x8F:
        return (seq('L01 S00'), seq('S03 S00'))            # taken, not
    if 0xA4 <= opc <= 0xAF:
        return (seq('L1C L1B S00'),)
    if 0xB4 <= opc <= 0xBF:
        return (seq('L1C L1A S00'),)
    return (seq('S00'),)


def patterns(args=None):
    out = []
    for opc, (mnemo, opr, length, cyc) in sorted(table().items()):
        code = [opc] + [NOP] * (length - 1)
        base = dict(key='%02X' % opc, mnemo=mnemo, operands=opr,
                    bytes=code + [NOP] * (PAD - len(code)))
        if 0x80 <= opc <= 0x87 or 0x90 <= opc <= 0x9F:
            # BT on S/C/Z, BF on S/C/Z/O: none set, then all.
            out.append(dict(base, key=base['key'] + ':w00', regs={'W': 0x00}))
            out.append(dict(base, key=base['key'] + ':w0f', regs={'W': 0x0F}))
        elif opc == 0x8F:
            # BR7 branches unless ISARL is 7.
            out.append(dict(base, key=base['key'] + ':isl0', regs={'ISL': 0}))
            out.append(dict(base, key=base['key'] + ':isl7', regs={'ISL': 7}))
        else:
            out.append(base)
    return out


def cycles(txt):
    """[kind, romc, data, space, addr, xtly, matched] per printed cycle;
    kind is ' ' for a cycle that neither reads nor writes memory or I/O,
    space 'A' for memory, 'I' for I/O, '' for neither."""
    out = []
    for m in re.finditer(r'^([RW ]) C=([0-9A-F]{2}) D=([0-9A-F]{2})'
                         r'(?: ([AI])=([0-9A-F]+))? +x=(\d+) m=(\d+)$', txt, re.M):
        out.append([m.group(1), int(m.group(2), 16), int(m.group(3), 16),
                    m.group(4) or '', int(m.group(5), 16) if m.group(5) else None,
                    int(m.group(6)), int(m.group(7))])
    return out


def fetches(trace):
    return [i for i, c in enumerate(trace) if c[1] == 0x00]


def record(run, cycles, ok):
    rec = dict(run, end='timeout' if not ok else 'other', cycles=cycles)
    f = fetches(cycles)
    if ok and len(f) == 2 and f[0] == 0 and f[1] == len(cycles) - 1 \
            and cycles[0][4] == ORG:
        rec['end'] = 'trap'
    return rec


def restore(run, cycles):
    return [(c[4], [NOP]) for c in cycles if c[0] == 'W' and c[3] == 'A']


def check(rec):
    opc = int(rec['key'][:2], 16)
    mnemo, opr, length, cyc = table()[opc]
    want = length + cyc
    head = '%-5s %-6s' % (mnemo, opr)
    trace = rec['cycles']
    if rec['end'] != 'trap':
        return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
    problems = []
    own = trace[1:]
    if trace[0][6] != want:
        problems.append('matcher gave %d cycles, the table %d' % (trace[0][6], want))
    if len(own) != want:
        problems.append('table says %d cycles, the chip %d' % (want, len(own)))
    # Without a transfer, the operand reads are the bytes after the opcode.
    operands = sorted({c[4] for c in own[:-1] if c[0] == 'R' and c[3] == 'A'
                       and ORG < c[4] < ORG + PAD})
    if operands != list(range(ORG + 1, ORG + length)):
        problems.append('read operands at %s, length %d'
                        % (' '.join('%04X' % a for a in operands), length))
    chip = [(c[5], c[1]) for c in own]
    alts = manual(opc)
    if alts is not None:
        romcs = [[r for _, r in alt] for alt in alts]
        if [r for _, r in chip] not in romcs:
            problems.append('ROMC %s, the data sheet %s' % (
                ' '.join('%02X' % r for _, r in chip),
                ' or '.join(' '.join('%02X' % r for r in alt) for alt in romcs)))
        else:
            alt = alts[romcs.index([r for _, r in chip])]
            if [x for x, _ in chip] != [XTLY[k] for k, _ in alt]:
                problems.append('XTLY %s, the data sheet %s' % (
                    ' '.join(str(x) for x, _ in chip),
                    ' '.join('%s%02X' % a for a in alt)))
    return '%s %s' % (head, '; '.join(problems)) if problems else None
