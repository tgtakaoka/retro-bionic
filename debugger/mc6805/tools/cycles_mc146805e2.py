"""MC146805E2 plugin for scripts/record-cycles.py; cycles_mc68hc05c0.py is
the same with the MC68HC05C0's table and memory map.

Each pattern is one opcode of the table, run from ORG in memory filled
with SWI ($83), so any transfer stops at once. Operands point into the
fill, never at on-chip RAM or I/O; RTS and RTI pop a stack seeded to
return into it. Needs the profile image (-D PROFILE_CYCLES), whose dump
prints every cycle, the ones the chip flags as opcode fetches (LI, #LIR)
as L: check() holds the run against the table's cycle count and length.

    P=debugger/mc6805/tools/cycles_mc146805e2.py
    R=debugger/mc6805/tools/mc146805e2-cycles.jsonl.zst
    C=samples/mc6805/bench/channels.toml:mc146805e2
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R --channels $C --capture debug
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SWI = 0x83
ORG = 0x1000
PAD = 4      # longer than any instruction
# Besides the relative branches, what may go elsewhere than the next opcode.
TRANSFER = {'JMP', 'JSR', 'RTS', 'RTI'}
# Cycles from the SWI's opcode fetch to the end of the ring: 1:N:w:W:W:W:W:V:v:n
TRAP = 10
RETURN = 0x18     # popped as CC (H and I set), A, X and PC 1818


def define(ns, name, boards, max_addr, internal, acia, sleep, skip, opr, stack,
           strobe, min_width):
    """Fill |ns| with the plugin for |name|.txt on |boards|. |internal|(addr)
    is true where the fill must not go: on-chip, or with side effects;
    |acia| is the emulated ACIA's range. |opr| gives X and the operand
    bytes: d8 direct, n8 an index offset, a16 extended. |stack| is the
    range SP can take.
    |strobe|(row, col) is the kind of the bus cycle a captured row is in,
    or None; a strobe shorter than |min_width| is crosstalk."""
    path = os.path.join(ARCH, '%s.txt' % name)
    vec_swi = max_addr - 3
    reset = max_addr - 1
    BASE = [('PC', ORG), ('X', opr['X']), ('A', SWI), ('CC', 0x08)]

    def fill_value(addr):
        if internal(addr) or addr in acia:
            return None
        if addr >= reset:
            return (ORG >> 8, ORG & 0xFF)[addr - reset]     # reset vector
        return SWI

    def ranges():
        out, start = [], None
        for addr in range(reset + 1):
            ok = addr < reset and fill_value(addr) == SWI
            if ok and start is None:
                start = addr
            elif not ok and start is not None:
                out.append((start, addr - start, [SWI]))
                start = None
        return out + [(reset, 1, [ORG >> 8, ORG & 0xFF])]

    def table():
        """{opc: (mnemonic, operands, cycles, length)}."""
        out = {}
        for line in open(path):
            f = line.split()
            if len(f) == 5 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[1] != '-':
                out[int(f[0], 16)] = (f[1], f[2], int(f[3]), int(f[4]))
        return out

    def operands(form):
        d8, n8, a16 = opr['d8'], opr['n8'], opr['a16']
        return {'-': [], ',X': [], '#n8': [SWI], 'r8': [SWI], 'd8': [d8],
                'b,d8': [d8], 'b,d8,r8': [d8, SWI], 'n8,X': [n8],
                'a16': [a16 >> 8, a16 & 0xFF],
                'a16,X': [a16 >> 8, a16 & 0xFF]}[form]

    def patterns(args=None):
        out = []
        for opc, (mnemo, form, clk, length) in sorted(table().items()):
            key = '%02X' % opc
            if opc == SWI or key in skip:
                continue
            code = [opc] + operands(form)
            assert len(code) == length, key
            # Every write is this long, so no longer earlier pattern lingers.
            code += [SWI] * (PAD - len(code))
            run = dict(key=key, mnemo=mnemo, operands=form, bytes=code)
            if mnemo in ('RTS', 'RTI'):
                run['data_seed'] = (stack.start, [RETURN] * len(stack))
            out.append(run)
        return out

    def cycles(txt):
        """[kind, addr, data, fetch] per printed cycle."""
        return [['R' if m.group(1) == 'L' else m.group(1), int(m.group(2), 16),
                 int(m.group(3), 16), m.group(1) == 'L']
                for m in re.finditer(r'^([LRW ]) A=([0-9A-F]{4}) D=([0-9A-F]{2})$',
                                     txt, re.M)]

    def trapped(cycles):
        """The ring ends with the SWI, up to the dummy read after its
        vector."""
        if len(cycles) < TRAP + 1 or cycles[0][1] != ORG or not cycles[0][3]:
            return False
        t = cycles[-TRAP:]
        return (t[0][3] and t[0][2] == SWI and all(c[0] == 'W' for c in t[2:7])
                and t[7][1] == vec_swi)

    def record(run, cycles, ok):
        end = 'timeout' if not ok else 'trap' if trapped(cycles) else 'other'
        return dict(run, end=end, cycles=cycles)

    def span(cycles):
        """The debug pin rises after the opcode fetch and falls before the
        SWI's vector read."""
        if not trapped(cycles):
            return None
        return [(c[0], c[2]) for c in cycles[1:-3]]

    def bus_cycles(rows, col):
        """A cycle per strobe, its data as the strobe ends. A strobe
        split by a blip under 50ns is one."""
        d = [col('AD%d' % i) for i in range(8)]
        pulses, kind, last, start = [], None, None, 0.0
        for row in rows:
            t = float(row[0])
            k = strobe(row, col)
            if k:
                if kind is None:
                    start = t
                    if pulses and t - pulses[-1][3] < 50e-9:
                        start = pulses.pop()[2]      # the same cycle, resumed
                kind, last = k, row
            elif kind:
                pulses.append((kind, sum(int(last[c]) << i for i, c in enumerate(d)),
                               start, t))
                kind = None
        return [(k, v) for k, v, start, stop in pulses if stop - start >= min_width]

    def restore(run, cycles):
        out = []
        for addr in sorted({c[1] for c in cycles if c[0] == 'W'}):
            value = fill_value(addr)
            if value is not None:
                out.append((addr, [value]))
        return out

    def check(rec):
        mnemo, form, clk, length = table()[int(rec['key'], 16)]
        head = '%-5s %-7s ~%-2d' % (mnemo, form, clk)
        trace = rec['cycles']
        expect = 'other' if rec['key'] in sleep else 'trap'
        if rec['end'] != expect:
            return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
        if rec['end'] != 'trap':
            return None
        n = len(trace) - TRAP       # from the opcode fetch to the SWI's
        problems = []
        marks = [i for i, c in enumerate(trace) if c[3]]
        if marks != [0, n]:
            problems.append('fetches at %s' % ' '.join('%04X' % trace[i][1] for i in marks))
        if n != clk:
            problems.append('table says %d cycles, the chip %d' % (clk, n))
        read = {c[1] for c in trace[1:n] if c[0] == 'R'}
        missing = [a for a in range(ORG + 1, ORG + length) if a not in read]
        if missing:
            problems.append('operand %s not read' % ' '.join('%04X' % a for a in missing))
        transfer = 'r8' in form or mnemo in TRANSFER
        if not transfer and trace[n][1] != ORG + length:
            problems.append('next opcode at %04X' % trace[n][1])
        return '%s %s' % (head, '; '.join(problems)) if problems else None

    ns.update(NAME=boards, ORG=ORG, FILL=ranges(), BASE=BASE, fill_value=fill_value,
              table=table, patterns=patterns, cycles=cycles, record=record,
              span=span, bus_cycles=bus_cycles, restore=restore, check=check)


def _e2_strobe(row, col):
    if row[col('DS')] != '1':
        return None
    return 'R' if row[col('R/#W')] == '1' else 'W'


# 13 address lines; on-chip I/O and RAM below 80, SP within 40-7F.
define(globals(), 'mc146805', ('MC146805E2',), max_addr=0x1FFF,
       internal=lambda addr: addr < 0x80, acia=range(0x17F8, 0x1800),
       sleep={'8E', '8F'}, skip=set(),
       opr={'X': SWI, 'd8': SWI, 'n8': SWI, 'a16': 0x8383},
       stack=range(0x40, 0x80), strobe=_e2_strobe, min_width=100e-9)
