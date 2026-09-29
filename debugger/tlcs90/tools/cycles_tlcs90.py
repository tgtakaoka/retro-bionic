"""TMP90C802 (TLCS-90) plugin for scripts/record-cycles.py.

Each pattern is one opcode of tlcs90-PAGE0.txt, or a prefix with one
opcode of PAGE1-3.txt, with SWI ($FF) as its operand bytes, run from ORG
in memory filled with SWI, so any transfer stops at once. Needs the
profile image (-D PROFILE_CYCLES), whose dump marks each cycle the
matcher took for an opcode fetch with the number of cycles it matched:
check() holds that against the run.

    P=debugger/tlcs90/tools/cycles_tlcs90.py
    R=debugger/tlcs90/tools/tlcs90-cycles.jsonl.zst
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

NAME = 'TMP90C802'
SWI = 0xFF
ORG = 0x1000
# On-chip RAM is FEC0-FFBF and I/O FFC0-FFEF; FFF0-FFFF is external again
# (TMP90C802A data sheet, 3.2 Memory Map).
INTERNAL = range(0xFEC0, 0xFFF0)
# Pops read the SWI fill, pushes land in it, pointers point into it; F
# keeps interrupts disabled.
BASE_SP = 0x0F00
BASE = [('PC', ORG), ('SP', BASE_SP), ('IX', 0x2000), ('IY', 0x2100),
        ('BC', 0x2200), ('DE', 0x2300), ('HL', 0x2400), ('A', SWI), ('F', 0x00)]
HALT = 0x01
VEC_SWI = 0x0010                # a HALT here stops the run at the SWI
FILL = [(0, VEC_SWI, [SWI]), (VEC_SWI, 1, [HALT]),
        (VEC_SWI + 1, INTERNAL.start - VEC_SWI - 1, [SWI]),
        (INTERNAL.stop, 0x10000 - INTERNAL.stop, [SWI])]
# The prefix, how many bytes it takes, and the page of its opcode.
PREFIX = {}
for p in range(0xE0, 0xFF):
    PREFIX[p] = ((1, 1, 1, 3, 1, 1, 1, 2)[(p - 0xE0) % 8] if p < 0xF0 else
                 (2, 2, 2, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1)[p - 0xF0],
                 (2 if p < 0xE8 else 3) if p < 0xF0 else
                 (2 if p < 0xF4 else 3 if p < 0xF8 else 1))
# One prefix per addressing shape: the others only name another register.
SHAPES = (0xE0, 0xE3, 0xE4, 0xE7, 0xF0, 0xF3,      # page 2
          0xE8, 0xEB, 0xEC, 0xEF, 0xF4, 0xF7,      # page 3
          0xF8, 0xFE)                              # page 1
CONDITIONAL = re.compile(r'^(JP|JR|CALL|RET|DJNZ)$')


def lo(v):
    return v & 0xFF


def hi(v):
    return v >> 8


def fill_value(addr):
    if addr in INTERNAL:
        return None
    return HALT if addr == VEC_SWI else SWI


def table(page):
    """{opc: (mnemonic, operands, length, true_seq, false_seq)}."""
    out = {}
    for line in open(os.path.join(ARCH, 'tlcs90-PAGE%d.txt' % page)):
        f = line.split()
        if len(f) == 7 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[1] != '-':
            out[int(f[0], 16)] = (f[1], f[2], int(f[3]), f[5], f[6])
    return out


def prefix_seq(prefix):
    """The sequence of |prefix|'s own bytes, from its PAGE0 row; table()
    leaves the prefixes out, having no mnemonic."""
    for line in open(os.path.join(ARCH, 'tlcs90-PAGE0.txt')):
        f = line.split()
        if f and f[0] == '%02X' % prefix:
            return f[5]
    raise KeyError(prefix)


def patterns(args=None):
    out = []
    page0 = table(0)
    for opc, (mnemo, opr, length, seq, alt) in sorted(page0.items()):
        if opc == SWI:
            continue
        code = [opc] + [SWI] * (length - 1)
        if mnemo == 'LD' and opr == 'SP,mn':
            # The fill would put the stack at FFFF, where the SWI's pushes
            # land in on-chip I/O and wedge the board.
            code = [opc, lo(BASE_SP), hi(BASE_SP)]
        out.extend(_runs('0:%02X' % opc, mnemo, opr, code, seq, alt))
    pages = {page: table(page) for page in (1, 2, 3)}
    for prefix in SHAPES:
        plen, page = PREFIX[prefix]
        for opc, (mnemo, opr, length, seq, alt) in sorted(pages[page].items()):
            if (mnemo, opr) in (('LD', 'SP,(gg)'), ('EX', '(gg),SP')):
                continue    # SP from memory: on chip or the fill, under most shapes
            code = [prefix] + [SWI] * (plen - 1) + [opc] + [SWI] * (length - 1)
            if page == 3 and (mnemo, opr) == ('LD', 'SP,gg'):
                # SP takes the effective address: (mn) is set to the base
                # SP, and (d) is always on chip.
                if prefix == 0xEF:
                    continue
                if prefix == 0xEB:
                    code = [prefix, lo(BASE_SP), hi(BASE_SP), opc]
            out.extend(_runs('%02X:%02X' % (prefix, opc), mnemo, opr, code, seq, alt))
    return out


PAD = 8      # every write is this long, so no longer earlier pattern lingers


def _runs(key, mnemo, opr, code, seq, alt):
    base = dict(key=key, mnemo=mnemo, operands=opr,
                bytes=code + [SWI] * (PAD - len(code)))
    if mnemo in ('LDIR', 'LDDR', 'CPIR', 'CPDR'):
        base['regs'] = {'BC': 1}    # one round, not BC's 2200: each round refetches
    if CONDITIONAL.match(mnemo) and alt != '-':
        return [dict(base, key=key + ':f00', regs={'F': 0x00}),
                dict(base, key=key + ':fdf', regs={'F': 0xDF})]   # all but I
    return [base]


def cycles(txt):
    """[kind, addr, data, matched] per printed cycle; kind is ' ' for a
    cycle that neither reads nor writes."""
    return [[m.group(1), int(m.group(2), 16), int(m.group(3), 16), int(m.group(4))]
            for m in re.finditer(r'^([RW ]) A=([0-9A-F]{4}) D=([0-9A-F]{2}) m=(\d+)',
                                 txt, re.M)]


def record(run, cycles, ok):
    """The pattern's own cycles: from its opcode fetch up to the SWI's own.
    The SWI's opcode comes in as the pattern's prefetch, and the pattern's
    last cycles may follow it; the SWI's own begin with a read of its
    address + 1, then of 0010, then its four pushes."""
    rec = dict(run, end='timeout' if not ok else 'other', cycles=cycles)
    v = next((i for i in range(2, len(cycles) - 4)
              if cycles[i][0] == 'R' and cycles[i][1] == VEC_SWI
              and all(c[0] == 'W' for c in cycles[i + 1:i + 5])), None)
    if ok and v is not None:
        dummy = cycles[v - 1]
        fetch = next((i for i in range(v - 2, -1, -1)
                      if cycles[i][0] == 'R'
                      and cycles[i][1] == (dummy[1] - 1) & 0xFFFF), None)
        if fetch is not None and cycles[fetch][2] == SWI:
            rec.update(end='trap', cycles=cycles[:v - 1], prefetch=fetch)
    return rec


def restore(run, cycles):
    out = []
    for addr in sorted({c[1] for c in cycles if c[0] == 'W'}):
        value = fill_value(addr)
        if value is not None:
            out.append((addr, [value]))
    return out


def _seq_cycles(seq):
    """Bus cycles after the prefetched opcode."""
    # d is an idle cycle; E/F reach FFFF with the fill's FF, an external
    # address.
    return len([c for c in seq.split(':') if c not in ('0', 'd')])


def check(rec):
    head = '%-5s %-9s' % (rec['mnemo'], rec['operands'])
    trace = rec['cycles']
    if rec['key'] == '0:01':            # HALT waits for an interrupt
        return None if rec['end'] == 'other' else '%s ended %s' % (head, rec['end'])
    if rec['end'] != 'trap':
        return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
    n = len(trace) - 1
    page0 = table(0)
    prefix, opc = (int(x, 16) for x in rec['key'].split(':')[:2])
    if prefix in PREFIX and rec['key'].count(':') >= 1 and rec['key'][0] != '0':
        pre = prefix_seq(prefix)
        row = table(PREFIX[prefix][1])[opc]
        want = {_seq_cycles(pre) + _seq_cycles(s) for s in row[3:5] if s != '-'}
    else:
        row = page0[opc]
        want = {_seq_cycles(s) for s in row[3:5] if s != '-'}
    problems = []
    # The matcher counts the cycles up to the next instruction's start: the
    # SWI's prefetch, when nothing of this one follows it.
    took = len(trace) - (1 if rec.get('prefetch') == len(trace) - 1 else 0)
    if trace[0][1] != ORG or trace[0][3] != took:
        problems.append('matcher took %d cycles, the chip %d' % (trace[0][3], took))
    if n not in want:
        problems.append('table says %s cycles, the chip %d'
                        % ('/'.join(map(str, sorted(want))), n))
    return '%s %s' % (head, '; '.join(problems)) if problems else None
