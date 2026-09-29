"""P8095BH (i8096) plugin for scripts/record-cycles.py.

Each pattern is one opcode of i8096-PAGE00.txt or, after FE, of
PAGEFE.txt, with TRAP ($F7) as its operand bytes, run from ORG in memory
filled with TRAP, so any transfer stops at once. Needs the profile image
(-D PROFILE_CYCLES), whose dump prints every cycle and the matcher's
fetches as I. The prefetch queue reads ahead of execution and the tables
give no cycle counts, so check() holds only the matcher's fetches against
the run: the pattern's opcode, and nothing else, before the TRAP's.

    P=debugger/i8096/tools/cycles_i8096.py
    R=debugger/i8096/tools/i8096-cycles.jsonl.zst
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

NAME = 'P8095BH'
TRAP = 0xF7
ORG = 0x3000
INTERNAL = 0x0100               # the register file
USART = range(0x0200, 0x0210)
CCB = 0x2018                    # read at reset
# PSW keeps interrupts disabled.
BASE = [('PC', ORG), ('SP', 0x0F00), ('PSW', 0x0000)]
FILL = [(INTERNAL, USART.start - INTERNAL, [TRAP]),
        (USART.stop, CCB - USART.stop, [TRAP]),
        (CCB + 1, 0x10000 - CCB - 1, [TRAP])]


def fill_value(addr):
    if addr < INTERNAL or addr in USART or addr == CCB:
        return None
    return TRAP


def table(page):
    """{opc: (mnemonic, operands, lengths, sequence)}; an n[w] row has two
    lengths, short- then long-indexed."""
    out = {}
    for line in open(os.path.join(ARCH, 'i8096-PAGE%s.txt' % page)):
        f = line.split()
        if len(f) >= 5 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[1] != '-':
            out[int(f[0], 16)] = (f[1], f[2], [int(n) for n in f[3].split('/')],
                                  f[5] if len(f) > 5 else '-')
    return out


# [w] and n[w] read their pointer register's word; F7 from the fill is odd,
# which makes [w] auto-increment and n[w] the long form, through an unset
# register. An even register set to an address in the fill instead: plain
# external memory, clear of I/O, on-chip RAM, the vectors and the CCB.
PAD = 8
VEC_TRAP = 0x2010
# The long form sets the register byte's LSB and takes a 16-bit
# displacement.
POINTER = 0x30
POINTS_AT = 0x2400
assert POINTS_AT >= 0x2100 and POINTS_AT not in USART


def patterns(args=None):
    out = []
    for page, prefix in (('00', []), ('FE', [0xFE])):
        for opc, (mnemo, opr, lengths, seq) in sorted(table(page).items()):
            if page == '00' and opc in (TRAP, 0xFE):
                continue
            if mnemo == 'RST':
                continue    # a reset: it restarts at 2080, never at a TRAP
            forms = [('', lengths[0])]
            if 'n[' in opr:
                forms.append((':long', lengths[1]))
            for suffix, length in forms:
                out += variants(page, prefix, opc, mnemo, opr, length, seq, suffix)
    return out


def variants(page, prefix, opc, mnemo, opr, length, seq, suffix):
    # The FE page's lengths count the prefix.
    code = prefix + [opc] + [TRAP] * (length - 1 - len(prefix))
    base = dict(key='%s:%02X%s' % (page, opc, suffix), mnemo=mnemo, operands=opr,
                bytes=code)
    if '[' in opr:
        # The pointer register is the first operand byte, then an n[w]'s
        # displacement: n + [w] must be even for a word access.
        at = len(prefix) + 1
        if suffix == ':long':
            code[at:at + 3] = [POINTER | 1, 0x02, 0x00]
        else:
            code[at] = POINTER
            if 'n[' in opr:
                code[at + 1] = 0x02
        base['data_seed'] = (POINTER, [POINTS_AT & 0xFF, POINTS_AT >> 8])
    # Every write is this long, so no longer earlier pattern lingers.
    code += [TRAP] * (PAD - len(code))
    if '@' in seq:
        # A conditional jump: once with every flag clear, once set but I.
        return [dict(base, key=base['key'] + ':psw0', regs={'PSW': 0x0000}),
                dict(base, key=base['key'] + ':pswf', regs={'PSW': 0xFD00})]
    return [base]


def cycles(txt):
    """[kind, addr, data, fetch] per printed cycle."""
    return [['R' if m.group(1) == 'I' else m.group(1), int(m.group(2), 16),
             int(m.group(3), 16), m.group(1) == 'I']
            for m in re.finditer(r'^([IRW]) A=([0-9A-F]{4}) D=([0-9A-F]{2})', txt, re.M)]


def record(run, cycles, ok):
    """The pattern's own cycles, up to the TRAP's vector read: the TRAP's
    opcode is prefetched, so the pattern's last cycles may follow it."""
    rec = dict(run, end='timeout' if not ok else 'other', cycles=cycles)
    vector = [i for i, c in enumerate(cycles) if c[0] == 'R' and c[1] == VEC_TRAP]
    if ok and vector:
        rec.update(end='trap', cycles=cycles[:vector[0]])
    return rec


def restore(run, cycles):
    out = []
    for addr in sorted({c[1] for c in cycles if c[0] == 'W'}):
        value = fill_value(addr)
        if value is not None:
            out.append((addr, [value]))
    return out


def check(rec):
    head = '%-6s %-9s' % (rec['mnemo'], rec['operands'])
    trace = rec['cycles']
    if rec['end'] != 'trap':
        return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
    # The pattern's opcode, then the TRAP's.
    marks = [c for c in trace if c[3]]
    if [c[1] for c in marks[:1]] != [ORG] or len(marks) != 2 or marks[1][2] != TRAP:
        marks = [c[1] for c in marks]
        return '%s matcher marked fetches at %s' % (head, ' '.join('%04X' % a for a in marks))
    return None
