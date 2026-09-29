"""MC68HC08AZ0 plugin for scripts/record-cycles.py.

Each pattern is one opcode of mc68hc08-P00.txt/-P9E.txt with SWI ($83) as
its operand bytes, run from ORG in memory filled with SWI, so any transfer
stops at once. Needs the profile image (-D PROFILE_CYCLES). The
recording is kept as mc68hc08-cycles.jsonl.zst, beside this file:

    P=debugger/mc6805/tools/cycles_mc68hc08.py
    R=debugger/mc6805/tools/mc68hc08-cycles.jsonl.zst
    C=samples/mc68hc08/bench/channels.toml:mc68hc08az0-profile
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R --channels $C --capture debug
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TABLES = {'00': 'mc68hc08-P00.txt', '9E': 'mc68hc08-P9E.txt'}

NAME = 'MC68HC08AZ0'
SWI = 0x83
ORG = 0x1000
ACIA = range(0xFFE0, 0xFFF0)
# Pops read the SWI fill, pushes stay in external memory, and indexed
# operands land in the fill.
BASE = [('PC', ORG), ('SP', 0x0EFF), ('HX', 0x8383), ('A', SWI)]
SKIP = {'00:83', '00:9E'}  # SWI itself, the 9E prefix


def internal(addr):
    """Mirrors PinsMc68HC08AZ0::is_internal(): not served by the debugger."""
    if 0x0A00 <= addr < 0xFE00 or addr >= 0xFF00:
        return False
    if 0x0450 <= addr < 0x0500 or 0x0580 <= addr < 0x0800:
        return False
    if 0xFE10 <= addr < 0xFE1C:
        return False
    return True


def fill_value(addr):
    if internal(addr) or addr in ACIA:
        return None
    if addr >= 0xFFFE:
        return (ORG >> 8, ORG & 0xFF)[addr & 1]     # reset vector
    return SWI


def _ranges():
    out, start = [], None
    for addr in range(0x10001):
        ok = addr < 0x10000 and fill_value(addr) == SWI
        if ok and start is None:
            start = addr
        elif not ok and start is not None:
            out.append((start, addr - start, [SWI]))
            start = None
    return out + [(0xFFFE, 1, [ORG >> 8, ORG & 0xFF])]


FILL = _ranges()


def table():
    """{key: (mnemonic, operands, length, sequence)}."""
    out = {}
    for page, name in TABLES.items():
        for line in open(os.path.join(ARCH, name)):
            f = line.split()
            if len(f) == 6 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[3] != '-':
                out['%s:%s' % (page, f[0])] = (f[1], f[2], int(f[3]), f[5])
    return out


def patterns(args=None):
    for key, (mnemo, opr, length, seq) in sorted(table().items()):
        if key in SKIP:
            continue
        page, opc = key.split(':')
        code = ([0x9E] if page == '9E' else []) + [int(opc, 16)]
        yield dict(key=key, mnemo=mnemo, operands=opr, seq=seq,
                   bytes=code + [SWI] * (length - len(code)))


def record(run, cycles, ok):
    """The pattern's own cycles: without the opcode fetch the run starts
    with, and the SWI's read and five pushes it ends with."""
    body = cycles[1:] if cycles and cycles[0][1] == ORG else cycles
    end = 'other'
    if not ok:
        end = 'timeout'
    elif len(body) >= 6 and all(c[0] == 'W' for c in body[-5:]):
        body, end = body[:-6], 'trap'
    return dict(run, end=end, cycles=body)


def span(cycles):
    """The debug pin rises after the opcode fetch and falls before the
    SWI's vector read."""
    return [(kind, data) for kind, _, data in cycles[1:]]


def restore(run, cycles):
    out = []
    for addr in sorted({addr for kind, addr, _ in cycles if kind == 'W'}):
        value = fill_value(addr)
        if value is not None:
            out.append((addr, [value]))
    return out


def bus_cycles(rows, col):
    """REB or WEB low for a bus cycle, about 600ns. Crosstalk shows as
    pulses under 100ns, and as high blips of a few ns splitting a cycle,
    whose halves are merged."""
    ed = [col('ED%d' % i) for i in range(8)]
    reb, web = col('REB'), col('WEB')
    pulses, kind, data, start = [], None, 0, 0.0
    for row in rows:
        t = float(row[0])
        if row[reb] == '0' or row[web] == '0':
            if kind is None:
                start = t
                if pulses and t - pulses[-1][3] < 50e-9:
                    start = pulses.pop()[2]      # the same cycle, resumed
            kind = 'R' if row[reb] == '0' else 'W'
            data = sum(int(row[c]) << i for i, c in enumerate(ed))
        elif kind:
            pulses.append((kind, data, start, t))
            kind = None
    return [(k, d) for k, d, start, stop in pulses if stop - start >= 100e-9]


# Which recorded cycles each table letter accepts: p the instruction's own
# operand bytes, n the next opcode, l the byte after it, s the previous
# address again, r any other read, w a write.
ACCEPTS = {'2': 'p', '3': 'p', '4': 'p', 'N': 'ns', 'd': 'snl',
           'J': 'pnlsr', 'j': 'pnlsr', 'i': 'pnlsr',
           'R': 'r', 'r': 'r', 'A': 'r', 'D': 'r',
           'W': 'w', 'w': 'w', 'B': 'w', 'E': 'w', 'V': 'r', 'v': 'r'}


def _chip_code(trace, length):
    code, prev = '', ORG
    nxt = ORG + length
    for kind, addr, _ in trace:
        if kind == 'W':
            code += 'w'
        elif addr == prev:
            code += 's'
        elif ORG < addr < nxt:
            code += 'p'
        elif addr == nxt:
            code += 'n'
        elif addr == nxt + 1:
            code += 'l'
        else:
            code += 'r'
        prev = addr
    return code


SLEEP = {'00:8E', '00:8F'}  # STOP and WAIT: no bus cycle after their prefetch
_TABLE = None


def check(rec):
    """The recording against the current tables."""
    global _TABLE
    _TABLE = _TABLE or table()
    mnemo, opr, length, seq = _TABLE[rec['key']]
    want = seq.split(':')[1:]
    got = _chip_code(rec['cycles'], len(rec['bytes']))
    head = '%-5s %-9s %-22s' % (mnemo, opr, seq)
    if len(rec['bytes']) != length:
        return '%s recorded with %d bytes' % (head, len(rec['bytes']))
    if rec['end'] != ('other' if rec['key'] in SLEEP else 'trap'):
        return '%s ended %s: chip %s' % (head, rec['end'], got)
    if len(got) != len(want) or any(g not in ACCEPTS.get(w, '') for g, w in zip(got, want)):
        return '%s chip %s' % (head, got)
    return None
