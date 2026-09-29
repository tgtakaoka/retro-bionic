"""MC6800 plugin for scripts/record-cycles.py, for the MC6800 and MC6802
boards; cycles_mb8861.py, cycles_mc6801.py and cycles_hd6301.py are the
same with their own table.

Each pattern is one opcode of the table with SWI ($3F) as its operand
bytes, run from ORG in memory filled with SWI, so any transfer stops at
once. Needs the profile image (-D PROFILE_CYCLES), whose dump marks
each cycle the matcher took for an opcode fetch with the number of cycles
it matched: check() holds that against the run.

    P=debugger/mc6800/tools/cycles_mc6800.py
    R=debugger/mc6800/tools/mc6800-cycles.jsonl.zst
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ARCH = os.path.dirname(HERE)


def define(ns, name, boards, internal, prefetch=False, regs16=False):
    """Fill |ns| with the plugin for |name|.txt on |boards|. Below
    |internal| the debugger doesn't serve memory. With |prefetch|, the
    table's sequences start after an opcode the previous instruction
    fetched (0:) rather than with its fetch (1:)."""
    path = os.path.join(ARCH, '%s.txt' % name)
    SWI = 0x3F
    ORG = 0x1000
    ACIA = range(0xDF00, 0xDF10)
    # Pulls read the SWI fill, pushes land in it, X points into it.
    BASE = [('PC', ORG), ('SP', 0x0F00), ('X', 0x2000)] + \
        ([('D', 0x3F3F)] if regs16 else [('A', SWI), ('B', SWI)]) + [('CC', 0x10)]
    SLEEP = {'3E', '1A'} if prefetch else {'3E'}   # WAI, and SLP, wait for an interrupt
    BRANCH = re.compile(r'^B(RA|RN|HI|LS|CC|CS|NE|EQ|VC|VS|PL|MI|GE|LT|GT|LE)$')
    # Cycles from the SWI's opcode fetch to its first push.
    LEAD = 3 if prefetch else 2

    def fill_value(addr):
        if addr < internal or addr in ACIA:
            return None
        if addr >= 0xFFFE:
            return (ORG >> 8, ORG & 0xFF)[addr & 1]     # reset vector
        return SWI

    FILL = [(internal, ACIA.start - internal, [SWI]),
            (ACIA.stop, 0xFFFE - ACIA.stop, [SWI]),
            (0xFFFE, 1, [ORG >> 8, ORG & 0xFF])]

    def table():
        """{opc: (mnemonic, operands, cycles, length, sequence)}."""
        out = {}
        for line in open(path):
            f = line.split()
            if len(f) == 6 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[5] != '-':
                out[int(f[0], 16)] = (f[1], f[2], int(f[3]), int(f[4]), f[5])
        return out

    def patterns(args=None):
        out = []
        for opc, (mnemo, opr, clk, length, seq) in sorted(table().items()):
            if opc == SWI:
                continue
            base = dict(key='%02X' % opc, mnemo=mnemo, operands=opr, opc=opc,
                        bytes=[opc] + [SWI] * (length - 1))
            if BRANCH.match(mnemo):
                for cc, tag in ((0x10, 'cc0'), (0x1F, 'ccf')):
                    out.append(dict(base, key='%02X:%s' % (opc, tag), regs={'CC': cc}))
            else:
                out.append(base)
        return out

    def cycles(txt):
        """[kind, addr, data, fetch, matched] per printed cycle; data is
        None for a non-VMA cycle."""
        out = []
        for m in re.finditer(r'^([FRW]) A=([0-9A-F]{4})(?: D=([0-9A-F]{2}))? m=(\d+)',
                             txt, re.M):
            out.append(['W' if m.group(1) == 'W' else 'R', int(m.group(2), 16),
                        int(m.group(3), 16) if m.group(3) else None,
                        m.group(1) == 'F', int(m.group(4))])
        return out

    def record(run, cycles, ok):
        """The pattern's own cycles, up to the SWI's opcode fetch."""
        rec = dict(run, end='timeout' if not ok else 'other', cycles=cycles)
        vec = next((i for i, c in enumerate(cycles)
                    if c[0] == 'R' and c[1] == 0xFFFA), None) if ok else None
        if vec is not None:
            j = vec
            while j > 0 and cycles[j - 1][0] != 'W':
                j -= 1                          # non-VMA, dummy read
            while j > 0 and cycles[j - 1][0] == 'W':
                j -= 1                          # the pushes
            fetch, end = j - LEAD, j - LEAD + 1
            if prefetch:
                # Dummy cycles may follow the SWI's prefetch, and are this
                # instruction's: it ends where the SWI's own first read,
                # past its opcode, begins.
                after = ORG + len(run['bytes'])
                fetch = next((i for i, c in enumerate(cycles[:j])
                              if c[0] == 'R' and c[1] == after), -1)
                end = next((i for i, c in enumerate(cycles[:j])
                            if c[0] == 'R' and c[1] == after + 1), -1)
            if fetch >= 0 and end > fetch and cycles[fetch][2] == SWI:
                rec.update(end='trap', cycles=cycles[:end])
        return rec

    def span(cycles):
        """The debug pin rises before the opcode fetch and falls as the
        SWI's vector read ends, too close to call: up to the cycle before."""
        vec = next((i for i, c in enumerate(cycles)
                    if c[0] == 'R' and c[1] == 0xFFFA), None)
        if vec is None:
            return None                 # stopped by an NMI, after the span
        return [(c[0], c[2]) for c in cycles[:vec]]

    def bus_cycles(rows, col):
        """R/#W and data where PHI2 (MC6800) or E falls, after it was high
        for at least 100ns: shorter pulses are crosstalk. A cycle with VMA
        low has no data. A fall on the span's last sample is the vector
        read, which span() leaves out."""
        d = [col('D%d' % i) for i in range(8)]
        try:
            clock = col('PHI2')
        except KeyError:
            clock = col('E')
        rw = col('R/#W')
        try:
            vma = col('VMA')
        except KeyError:
            vma = None
        out, prev, rise = [], None, None
        for n, row in enumerate(rows):
            if prev is not None and prev[clock] == '0' and row[clock] == '1':
                rise = float(row[0])
            elif prev is not None and prev[clock] == '1' and row[clock] == '0':
                if rise is not None and float(row[0]) - rise >= 100e-9 \
                        and n != len(rows) - 1:
                    valid = vma is None or prev[vma] == '1'
                    out.append(('R' if prev[rw] == '1' else 'W',
                                sum(int(prev[c]) << i for i, c in enumerate(d))
                                if valid else None))
            prev = row
        return out

    def restore(run, cycles):
        out = []
        for addr in sorted({c[1] for c in cycles if c[0] == 'W'}):
            value = fill_value(addr)
            if value is not None:
                out.append((addr, [value]))
        return out

    def check(rec):
        mnemo, opr, clk, length, seq = table()[rec['opc']]
        head = '%-5s %-6s %-24s' % (mnemo, opr, seq)
        trace = rec['cycles']
        expect = 'other' if rec['key'].split(':')[0] in SLEEP else 'trap'
        if rec['end'] != expect:
            return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
        if rec['end'] != 'trap':
            return None
        # The run starts with the opcode's fetch, and ends with the SWI's,
        # which a prefetching CPU's table counts as this one's N instead.
        n = len(trace) - 1
        problems = []
        if not prefetch and (trace[0][1] != ORG or trace[0][4] != n):
            problems.append('matcher took %d cycles, the chip %d' % (trace[0][4], n))
        if '@' not in seq and clk != n:
            problems.append('table says %d cycles, the chip %d' % (clk, n))
        return '%s %s' % (head, '; '.join(problems)) if problems else None

    ns.update(NAME=boards, ORG=ORG, FILL=FILL, BASE=BASE, fill_value=fill_value,
              table=table, patterns=patterns, cycles=cycles, record=record,
              span=span, bus_cycles=bus_cycles, restore=restore, check=check)


# The MC6802's own RAM, when enabled, is 0000-007F.
define(globals(), 'mc6800', ('MC6800', 'MC6802'), internal=0x80)
