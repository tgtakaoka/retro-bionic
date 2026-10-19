"""P8095BH (i8096) plugin for scripts/record-cycles.py.

Each pattern is one opcode of i8096-PAGE00.txt or, after FE, of
i8096-PAGEFE.txt, with TRAP ($F7) as its operand bytes, run from ORG in
memory filled with TRAP, so any transfer stops at once. Needs the profile
image (-D PROFILE_CYCLES), whose dump prints every cycle and the matcher's
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

NAME = ('P8095BH', 'N8097BH', 'S80C196KC', 'S80C196KB', 'S80296SA')
TRAP = 0xF7
ORG = 0x3000
INTERNAL = 0x0100               # the register file
USART = range(0x0400, 0x0410)
CCB = 0x2018                    # read at reset
CCB1 = 0x201A                   # the 80296SA's, read next
# PSW keeps interrupts disabled.
BASE = [('PC', ORG), ('SP', 0x0F00), ('PSW', 0x0000)]


def fills():
    return [(INTERNAL, USART.start - INTERNAL, [TRAP]),
            (USART.stop, CCB - USART.stop, [TRAP]),
            (CCB + 1, CCB1 - CCB - 1, [TRAP]),
            (CCB1 + 1, 0x10000 - CCB1 - 1, [TRAP])]


FILL = fills()


# An 80C196 pushes the PC before it reads a vector; the 80296SA after.
PUSH_BEFORE_VECTOR = False
# The rows of the 80296SA's own instructions run on it only.
SA = False


def setup(b):
    """The S80C196KC's and 80296SA's register RAM reaches 1FFH: the fill
    starts above it."""
    global INTERNAL, FILL, PUSH_BEFORE_VECTOR, SA
    PUSH_BEFORE_VECTOR = b.who in ('S80C196KB', 'S80C196KC')
    SA = b.who == 'S80296SA'
    if b.who in ('S80C196KC', 'S80296SA'):
        INTERNAL = 0x0200
        FILL = fills()


def fill_value(addr):
    if addr < INTERNAL or addr in USART or addr in (CCB, CCB1):
        return None
    return TRAP


def table(page):
    """{opc: (mnemonic, operands, lengths, sequence, cpu)}; an n[w] row has
    two lengths, short- then long-indexed."""
    out = {}
    for line in open(os.path.join(ARCH, 'i8096-PAGE%s.txt' % page)):
        f = line.split()
        if len(f) >= 5 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[1] != '-':
            out[int(f[0], 16)] = (f[1], f[2], [int(n) for n in f[3].split('/')],
                                  f[5] if len(f) > 5 else '-',
                                  f[6] if len(f) > 6 else '')
    return out


# [w] and n[w] read their pointer register's word; F7 from the fill is odd,
# which makes [w] auto-increment and n[w] the long form, through an unset
# register. An even register set to an address in the fill instead: plain
# external memory, clear of I/O, on-chip RAM, the vectors and the CCB.
PAD = 8
VEC_TRAP = 0x2010               # TRAP's vector
# The long form sets the register byte's LSB and takes a 16-bit
# displacement.
POINTER = 0x30
POINTS_AT = 0x2400
# BMOV's and BMOVI's pointer pair, then their count.
BLOCK = 0x38
assert POINTS_AT >= 0x2100 and POINTS_AT not in USART


def patterns(args=None):
    out = []
    for page, prefix in (('00', []), ('FE', [0xFE])):
        for opc, (mnemo, opr, lengths, seq, cpu) in sorted(table(page).items()):
            if page == '00' and opc in (TRAP, 0xFE):
                continue
            if (cpu == '80296') != SA and (cpu == '80296' or mnemo in ('DPTS', 'EPTS')):
                continue    # the 80296SA's own, or the PTS it lacks
            if mnemo in ('RST', 'IDLPD'):
                continue    # a reset, as IDLPD's is with the fill's key: it
                            # restarts at 2080, never at a TRAP
            forms = [('', lengths[0])]
            if 'n[' in opr:
                forms.append((':long', lengths[1]))
            for suffix, length in forms:
                out += variants(page, prefix, opc, mnemo, opr, length, seq, suffix)
    if SA:
        out += sa_variants()
    return out


# The 80296SA's instructions told apart from older ones by an operand
# byte's bits: a signed MUL or DIV's odd destination, an EBR's odd
# register, MVAC's and MSAC's lreg bits, the MAC family's destination
# byte, RPT's 04H, here repeating a NOP.
SA_FORMS = [
    ('MUL:sgn', [0x6C, 0x1E, 0x21]),
    ('MUL3:sgn', [0x4C, 0x1C, 0x1E, 0x21]),
    ('DIV:sgn', [0x8C, 0x1E, 0x21]),
    ('MULB:sgn', [0x7C, 0x1E, 0x21]),
    ('MVAC', [0x0D, 0x0A, 0x21]),
    ('MSAC', [0x0D, 0x0A, 0x23]),
    ('MAC', [0x6C, 0x1C, 0x00]),
    ('MAC3', [0x4C, 0x1C, 0x1E, 0x00]),
    ('MAC:in', [0x6E, POINTER, 0x00]),
    ('MACR:in', [0x6E, POINTER, 0x04]),
    ('SMAC3', [0x4C, 0x1C, 0x1E, 0x01]),
    ('RPT', [0x41, 0x02, 0x00, 0x00, 0x04, 0xFD]),
]


def sa_variants():
    out = []
    for name, code in SA_FORMS:
        base = dict(key='SA:%s' % name, mnemo=name.split(':')[0], operands='-',
                    bytes=code + [TRAP] * (PAD - len(code)), length=len(code))
        if POINTER in code[1:2]:
            base['data_seed'] = (POINTER, [POINTS_AT & 0xFF, POINTS_AT >> 8])
        out.append(base)
    # EBR to the fill, a 24-bit pointer in page FFH
    out.append(dict(key='SA:EBR', mnemo='EBR', operands='-', length=2,
                    bytes=[0xE3, POINTER | 1] + [TRAP] * (PAD - 2),
                    data_seed=(POINTER, [POINTS_AT & 0xFF, POINTS_AT >> 8, 0xFF, 0x00])))
    return out


def variants(page, prefix, opc, mnemo, opr, length, seq, suffix):
    # The FE page's lengths count the prefix.
    code = prefix + [opc] + [TRAP] * (length - 1 - len(prefix))
    base = dict(key='%s:%02X%s' % (page, opc, suffix), mnemo=mnemo, operands=opr,
                bytes=code)
    if opr == 'eee':
        # EJMP's and ECALL's 24-bit displacement, kept in the page
        code[len(prefix) + 1:len(prefix) + 4] = [0x10, 0x00, 0x00]
    elif '[l]' in opr:
        # An extended pointer, 24 bits in page FFH, then an e[l]'s 24-bit
        # displacement.
        at = len(prefix) + 1
        code[at] = POINTER
        if 'e[' in opr:
            code[at + 1:at + 4] = [0x02, 0x00, 0x00]
        base['data_seed'] = (POINTER, [POINTS_AT & 0xFF, POINTS_AT >> 8, 0xFF, 0x00])
    elif '[' in opr:
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
    if mnemo == 'EBMOVI':
        # The count word, then the extended source and destination
        # pointers, in page FFH.
        code[len(prefix) + 1:len(prefix) + 3] = [BLOCK + 8, BLOCK]
        base['data_seed'] = (BLOCK, [POINTS_AT & 0xFF, POINTS_AT >> 8, 0xFF, 0,
                                     (POINTS_AT + 0x10) & 0xFF,
                                     (POINTS_AT + 0x10) >> 8, 0xFF, 0, 2, 0])
    if mnemo in ('BMOV', 'BMOVI'):
        # The count word, then the source and destination pointers: two
        # words from one place in the fill to another.
        code[len(prefix) + 1:len(prefix) + 3] = [BLOCK + 4, BLOCK]
        base['data_seed'] = (BLOCK, [POINTS_AT & 0xFF, POINTS_AT >> 8,
                                     (POINTS_AT + 0x10) & 0xFF,
                                     (POINTS_AT + 0x10) >> 8, 2, 0])
    if SA and mnemo == 'TIJMP':
        # An even TBASE beside the index's pointer, at the same place in
        # the fill: the jump lands on a TRAP there.
        code[len(prefix) + 3] = POINTER + 4
        base['data_seed'] = (POINTER, [POINTS_AT & 0xFF, POINTS_AT >> 8, 0, 0,
                                       POINTS_AT & 0xFF, POINTS_AT >> 8])
    elif SA:
        even_operands(code, len(prefix) + 1, length, opr)
    # Every write is this long, so no longer earlier pattern lingers.
    code += [TRAP] * (PAD - len(code))
    if '@' in seq:
        # A conditional jump: once with every flag clear, once set but I.
        return [dict(base, key=base['key'] + ':psw0', regs={'PSW': 0x0000}),
                dict(base, key=base['key'] + ':pswf', regs={'PSW': 0xFD00})]
    return [base]


# An odd register selects another instruction on the 80296SA: signed MUL,
# MSAC, EBR. Its register operands get an even one; a displacement keeps
# the TRAP, so a transfer still stops at once.
REGISTER = 0x1C


def even_operands(code, start, length, opr):
    disp = {'r': 1, 'rr': 2, 'eee': 3}.get(opr.split(',')[-1], 0)
    for i in range(start, length - disp):
        if code[i] == TRAP:
            code[i] = REGISTER


def cycles(txt):
    """[kind, addr, data, fetch, bytes] per printed cycle: a word cycle's
    data has four digits."""
    return [['R' if m.group(1) == 'I' else m.group(1), int(m.group(2), 16),
             int(m.group(3), 16), m.group(1) == 'I', len(m.group(3)) // 2]
            for m in re.finditer(r'^([IRW]) A=([0-9A-F]{4}) D=([0-9A-F]{4}|[0-9A-F]{2})\b',
                                 txt, re.M)]


def record(run, cycles, ok):
    """The pattern's own cycles, up to the TRAP's vector read: the TRAP's
    opcode is prefetched, so the pattern's last cycles may follow it."""
    rec = dict(run, end='timeout' if not ok else 'other', cycles=cycles)
    vector = [i for i, c in enumerate(cycles) if c[0] == 'R' and c[1] == VEC_TRAP]
    if ok and vector:
        end = vector[0]
        # an 80C196's push before it goes too
        if PUSH_BEFORE_VECTOR and end and cycles[end - 1][0] == 'W':
            end -= 1
        rec.update(end='trap', cycles=cycles[:end])
    return rec


def restore(run, cycles):
    out = []
    for addr in sorted({c[1] for c in cycles if c[0] == 'W'}):
        value = fill_value(addr)
        if value is not None:
            out.append((addr, [value]))
    return out


def lanes(c):
    """The bytes a recorded cycle moved: a word's two, from its address."""
    return [c[2] & 0xFF, c[2] >> 8] if len(c) > 4 and c[4] == 2 else [c[2]]


def length(rec):
    """The pattern's instruction length, by its key."""
    if 'length' in rec:
        return rec['length']
    page, opc = rec['key'].split(':')[:2]
    lengths = table(page)[int(opc, 16)][2]
    return lengths[1] if rec['key'].endswith(':long') else lengths[0]


def check(rec):
    head = '%-6s %-9s' % (rec['mnemo'], rec['operands'])
    trace = rec['cycles']
    if rec['end'] != 'trap':
        return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
    # The pattern's opcode, then the TRAP's.
    marks = [c for c in trace if c[3]]
    if len(marks) == 1 and length(rec) == 1 and lanes(marks[0])[1:] == [TRAP]:
        marks.append(marks[0])  # one word brought the opcode and the TRAP
    if [c[1] for c in marks[:1]] != [ORG] or len(marks) != 2 or TRAP not in lanes(marks[1]):
        marks = [c[1] for c in marks]
        return '%s matcher marked fetches at %s' % (head, ' '.join('%04X' % a for a in marks))
    return None
