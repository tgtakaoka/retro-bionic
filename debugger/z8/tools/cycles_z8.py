"""Shared by cycles_z86.py and cycles_z88.py, the scripts/record-cycles.py
plugins for the Z8 family; not a plugin itself.

Each pattern is one opcode of the table with the trap as its operand
bytes, except where an operand must point somewhere safe, run from ORG in
memory filled with the trap, so any transfer stops at once. The trap is
the chip's HALT or WFI, which stops the CPU; the profile image
(-D PROFILE_CYCLES) prints every cycle up to then, marks each cycle the
matcher took for an opcode fetch with the number of cycles it gave it,
and resets the CPU. check() finds the trap's fetch at the address the
pattern transfers to, and holds the cycles before it against the table
and the matcher.
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

ORG = 0x1000
BASE_SP = 0x0F00                # pushes land in the fill below, pops read it
DATA = 0x2000                   # where @rr points
USART = range(0xFF00, 0xFF10)
PAD = 4     # the longest instruction; every write is this long, so no earlier pattern lingers
# pins_z8.cpp's profile loop gives up past MAX_CYCLES, and stops at
# SPIN_READS reads of one address in a row.
MAX_CYCLES = 96
SPIN_READS = 4

C, Z, S, V = 0x80, 0x40, 0x20, 0x10
CONDITION = {
    'F': lambda f: False,
    'LT': lambda f: bool(f & S) != bool(f & V),
    'LE': lambda f: bool(f & Z) or bool(f & S) != bool(f & V),
    'ULE': lambda f: bool(f & (C | Z)),
    'OV': lambda f: bool(f & V),
    'MI': lambda f: bool(f & S),
    'Z': lambda f: bool(f & Z),
    'C': lambda f: bool(f & C),
    'GE': lambda f: bool(f & S) == bool(f & V),
    'GT': lambda f: not (f & Z) and bool(f & S) == bool(f & V),
    'UGT': lambda f: not (f & (C | Z)),
    'NOV': lambda f: not (f & V),
    'PL': lambda f: not (f & S),
    'NE': lambda f: not (f & Z),
    'NZ': lambda f: not (f & Z),
    'NC': lambda f: not (f & C),
}


def rel(addr, disp):
    return (addr + (disp - 0x100 if disp & 0x80 else disp)) & 0xFFFF


def flags_for(cc):
    """[(FLAGS, taken)]: one value that takes |cc| and one that doesn't."""
    test = CONDITION[cc]
    out = {}
    for f in range(0, 0x100, 0x10):
        out.setdefault(test(f), f)
    return [(f, taken) for taken, f in sorted(out.items(), key=lambda kv: not kv[0])]


def define(ns, chip, boards, trap, base, seed, special, halts=(), tail=None):
    """Fill |ns| with the plugin for |chip|.txt on |boards|. |base| sets the
    registers, |seed| the register file (addr, values), before each run.
    special(opc, mnemo, opr, length) gives an opcode's runs as dicts with
    'bytes', 'target' (where the trap is fetched, None for |halts|), and
    optionally 'tag' and 'regs', or None for the default. |tail| is how
    many cycles the chip makes after the trap's fetch."""
    path = os.path.join(ARCH, '%s.txt' % chip)
    word = trap << 8 | trap

    def fill_value(addr):
        return None if addr in USART else trap

    FILL = [(0, USART.start, [trap]), (USART.stop, 0x10000 - USART.stop, [trap])]

    def table():
        """{opc: (mnemonic, operands, length, bus, stack, sequence)}."""
        out = {}
        for line in open(path):
            f = line.split()
            if len(f) == 8 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[1] != '-' \
                    and f[4] != '0':
                out[int(f[0], 16)] = (f[1], f[2], int(f[4]), int(f[5]), int(f[6]), f[7])
        return out

    def default(opc, mnemo, opr, length):
        code = [opc] + [trap] * (length - 1)
        after = ORG + length
        cc, _, dst = opr.rpartition(',')
        if mnemo == 'DJNZ':
            r = 'R%d' % (opc >> 4)
            return [dict(tag='r1', bytes=code, regs={r: 1}, target=after),
                    dict(tag='r2', bytes=code, regs={r: 2}, target=rel(after, trap))]
        if mnemo in ('JR', 'JP') and dst in ('RA', 'DA'):
            taken = rel(after, trap) if dst == 'RA' else word
            if not cc:
                return [dict(bytes=code, target=taken)]
            if cc == 'F':
                return [dict(bytes=code, target=after)]
            return [dict(tag='f%02x' % f, bytes=code, regs={'FLAGS': f},
                         target=taken if t else after) for f, t in flags_for(cc)]
        if mnemo in ('RET', 'IRET') or (mnemo, opr) == ('CALL', 'DA'):
            return [dict(bytes=code, target=word)]
        if opc in halts:
            return [dict(bytes=code, target=None)]
        return [dict(bytes=code, target=after)]

    def patterns(args=None):
        out = []
        for opc, (mnemo, opr, length, bus, stk, seq) in sorted(table().items()):
            if opc == trap:
                continue
            runs = special(opc, mnemo, opr, length) or default(opc, mnemo, opr, length)
            for r in runs:
                key = '%02X' % opc + (':' + r['tag'] if r.get('tag') else '')
                code = r['bytes'] + [trap] * (PAD - len(r['bytes']))
                run = dict(key=key, mnemo=mnemo, operands=opr, bytes=code,
                           target=r['target'], data_seed=seed)
                if r.get('regs'):
                    run['regs'] = r['regs']
                out.append(run)
        return out

    def schedule(runs):
        """What stops the CPU for good runs last."""
        return sorted(runs, key=lambda r: r['target'] is None)

    def cycles(txt):
        """[kind, addr, data, matched] per printed cycle."""
        return [[m.group(1), int(m.group(2), 16), int(m.group(3), 16), int(m.group(4))]
                for m in re.finditer(r'^([RW]) A=([0-9A-F]{4}) D=([0-9A-F]{2}) m=(\d+)',
                                     txt, re.M)]

    def record(run, cycles, ok):
        rec = dict(run, end='timeout' if not ok else 'other', cycles=cycles)
        if ok and len(cycles) > MAX_CYCLES:
            rec['end'] = 'cycles'
        elif ok and cycles and cycles[0][:3] == ['R', ORG, run['bytes'][0]]:
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
        mnemo, opr, length, bus, stk, seq = table()[opc]
        head = '%-6s %-9s' % (mnemo, opr)
        trace = list(rec['cycles'])
        if rec['end'] != 'trap':
            return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
        # The loop also stops at SPIN_READS reads of one address, which
        # would be WFI fetching in place: keep one of them.
        spin = 1
        while spin < len(trace) and trace[-1 - spin][:2] == trace[-1][:2] == ['R', trace[-1][1]]:
            spin += 1
        if spin >= SPIN_READS:
            del trace[len(trace) - spin + 1:]
        target = rec['target']
        if target is None:
            fetch = 0
        else:
            fetch = next((i for i in range(len(trace) - 1, 0, -1)
                          if trace[i][:3] == ['R', target, trap]), None)
            if fetch is None:
                return '%s no trap fetch at %04X in %d cycles' % (head, target, len(trace))
        problems = []
        if target is not None:
            want = length + bus + stk
            took = trace[0][3]
            if took != fetch or want != fetch:
                problems.append('matcher %d, %s.txt %d, chip %d cycles'
                                % (took, chip, want, fetch))
        if tail is not None and len(trace) - 1 - fetch != tail:
            problems.append('%d cycles after the trap fetch' % (len(trace) - 1 - fetch))
        return '%s %s' % (head, '; '.join(problems)) if problems else None

    ns.update(NAME=boards, ORG=ORG, FILL=FILL, BASE=base, fill_value=fill_value,
              table=table, patterns=patterns, schedule=schedule, cycles=cycles,
              record=record, restore=restore, check=check)
