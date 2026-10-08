"""MC6809 plugin for scripts/record-cycles.py, for the MC6809 and MC6809E.

Each pattern is one opcode of mc6809-P00/P10/P11.txt, with SWI ($3F) as its
operand bytes, run from ORG in memory filled with SWI, so any transfer
stops at once; LDA indexed runs once more for every postbyte of
mc6809-IX.txt. Needs the profile image (-D PROFILE_CYCLES), whose
dump marks each cycle the matcher took for an opcode fetch with the
number of cycles it matched: check() holds that against the run.

    P=debugger/mc6809/tools/cycles_mc6809.py
    R=debugger/mc6809/tools/mc6809-cycles.jsonl.zst
    C=samples/mc6809/bench/channels.toml:mc6809-profile
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R --channels $C --capture debug
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ARCH = os.path.dirname(HERE)


def define(ns, prefix, native):
    """Fill |ns| with the plugin for the tables named |prefix|; with
    |native|, every pattern runs in HD6309 native mode too."""
    tables = {p: os.path.join(ARCH, '%s-%s.txt' % (prefix, p))
              for p in ('P00', 'P10', 'P11', 'IX')}
    SWI = 0x3F
    ORG = 0x1000
    ACIA = range(0xDF00, 0xDF10)
    # Pulls read the SWI fill, pushes land in it, and every pointer
    # register points into it. CC keeps E set, F and I masked.
    BASE = [('PC', ORG), ('S', 0x0F00), ('U', 0x0E00), ('X', 0x2000),
            ('Y', 0x2100), ('D', 0x3F3F), ('DP', 0x3F), ('CC', 0xD0)]
    if native:
        # Every run sets the mode: LDMD changes it, and the debugger takes
        # it from the SWI's push.
        BASE.append(('MD', 0))
    SLEEP = {'P00:13', 'P00:3C'}   # SYNC and CWAI wait for an interrupt
    BRANCH = re.compile(r'^(B|LB)(RA|RN|HI|LS|CC|CS|NE|EQ|VC|VS|PL|MI|GE|LT|GT|LE)$')
    # Dividends whose quotient fits: the divisor is the fill.
    DIVIDE = {'DIVD': {'D': 0x0100}, 'DIVQ': {'D': 0x0000, 'W': 0x0100}}

    def fill_value(addr):
        if addr in ACIA:
            return None
        if addr >= 0xFFFE:
            return (ORG >> 8, ORG & 0xFF)[addr & 1]     # reset vector
        return SWI

    FILL = [(0, ACIA.start, [SWI]),
            (ACIA.stop, 0xFFFE - ACIA.stop, [SWI]),
            (0xFFFE, 1, [ORG >> 8, ORG & 0xFF])]

    def table(page):
        """{opc: (mnemonic, operands, cycles, length, sequence)}."""
        out = {}
        for line in open(tables[page]):
            f = line.split()
            if len(f) == 6 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[5] != '-':
                out[int(f[0], 16)] = (f[1], f[2], f[3], f[4], f[5])
        return out

    def number(text, mode):
        """The emulation (0) or native (1) figure of '6/5', '4+' or '3'."""
        part = text.split('/')
        m = re.match(r'(\d+)(\+?)', part[min(mode, len(part) - 1)])
        return (int(m.group(1)), m.group(2) == '+') if m else (0, True)

    def runs(page, opc, row, mode):
        mnemo, opr, clk, length, seq = row
        prefix = [] if page == 'P00' else [0x10 if page == 'P10' else 0x11]
        n, _ = number(length, 0)
        code = prefix + [opc] + [SWI] * (n - 1)
        key = '%s:%02X' % (page, opc)
        regs = {'MD': 1} if mode else {}
        if mnemo == 'TFM':
            code[2] = 0x12              # X to Y, two bytes
            regs['W'] = 2
        tag = ':native' if mode else ''
        base = dict(key=key + tag, mnemo=mnemo, operands=opr, page=page, opc=opc,
                    mode=mode, bytes=code, regs=regs)
        if BRANCH.match(mnemo):
            for cc, name in ((0xD0, 'cc0'), (0xDF, 'ccf')):
                yield dict(base, key='%s:%s%s' % (key, name, tag), regs=dict(regs, CC=cc))
        elif mnemo in DIVIDE:
            # Dividing the fill by itself overflows, which ends early; a
            # small dividend gives a quotient that fits.
            yield base
            yield dict(base, key='%s:fit%s' % (key, tag), regs=dict(regs, **DIVIDE[mnemo]))
        else:
            yield base

    def patterns(args=None):
        out = []
        modes = (0, 1) if native else (0,)
        ix = table('IX')
        for mode in modes:
            for page in ('P00', 'P10', 'P11'):
                for opc, row in sorted(table(page).items()):
                    if page == 'P00' and opc in (0x10, 0x11, SWI):
                        continue
                    out.extend(runs(page, opc, row, mode))
            # LDA indexed, for every postbyte
            for post, row in sorted(ix.items()):
                extra, _ = number(row[3], 0)
                regs = {'MD': 1} if mode else {}
                out.append(dict(key='IX:%02X%s' % (post, ':native' if mode else ''),
                                mnemo='LDA', operands=row[0], page='IX', opc=post,
                                mode=mode, bytes=[0xA6, post] + [SWI] * extra, regs=regs))
        return out

    def cycles(txt):
        """[kind, addr, data, status, cntl, matched] per printed cycle."""
        out = []
        for m in re.finditer(r'^([ VSH])([RW]) A=([0-9A-F]{4}) D=([0-9A-F]{2}) ([L ])'
                             r' c=([0-9A-F]) m=(\d+)', txt, re.M):
            out.append([m.group(2), int(m.group(3), 16), int(m.group(4), 16),
                        m.group(1), int(m.group(6), 16), int(m.group(7))])
        return out

    def _vector(cycles):
        for i, c in enumerate(cycles):
            if c[3] == 'V' and c[1] == 0xFFFA:
                return i
        return None

    def record(run, cycles, ok):
        """The pattern's own cycles: from its opcode fetch to the SWI's,
        which the matcher takes for the next instruction."""
        rec = dict(run, end='timeout' if not ok else 'other', cycles=cycles)
        vec = _vector(cycles) if ok else None
        if vec is not None:
            j = vec - 2                         # the x before V
            while j > 0 and cycles[j - 1][0] == 'W':
                j -= 1
            fetch = j - 3                       # SWI: 1:X:x:w:W...
            if fetch >= 0 and cycles[fetch][2] == SWI:
                rec.update(end='trap', cycles=cycles[:fetch + 1])
        return rec

    def span(cycles):
        """The debug pin rises before the opcode fetch and falls as the
        SWI's vector read ends, too close to call: up to the cycle before.
        Nothing drives the bus on a dummy read of $FFFF."""
        vec = _vector(cycles)
        if vec is None:
            return None                 # stopped by an NMI, after the span
        return [(c[0], None if c[0] == 'R' and c[1] == 0xFFFF else c[2])
                for c in cycles[:vec]]

    def bus_cycles(rows, col):
        """R/#W as E rises and data as E falls, after E high for a cycle's
        500ns: shorter pulses are crosstalk. The HD6309 drives R/#W back
        high right at E's fall, too close to sample there. An E fall on the
        span's last sample is the vector read, which span() leaves out."""
        d = [col('D%d' % i) for i in range(8)]
        e, rw = col('E'), col('R/#W')
        out, prev, rise, kind = [], None, None, None
        for n, row in enumerate(rows):
            if prev is not None and prev[e] == '0' and row[e] == '1':
                rise = float(row[0])
                kind = 'R' if row[rw] == '1' else 'W'
            elif prev is not None and prev[e] == '1' and row[e] == '0':
                if rise is not None and float(row[0]) - rise >= 100e-9 \
                        and n != len(rows) - 1:
                    out.append((kind, sum(int(prev[c]) << i for i, c in enumerate(d))))
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
        head = '%-6s %-9s' % (rec['mnemo'], rec['operands'])
        trace = rec['cycles']
        expect_end = 'other' if ':'.join(rec['key'].split(':')[:2]) in SLEEP else 'trap'
        if rec['end'] != expect_end:
            return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
        if rec['end'] != 'trap':
            return None
        n = len(trace) - 1              # without the next opcode fetch
        problems = []
        if trace[0][1] != ORG or trace[0][5] != n:
            problems.append('matcher took %d cycles, the chip %d'
                            % (trace[0][5], n))
        # The table's own count, where it is fixed.
        if rec['page'] == 'IX':
            base, _ = number(table('P00')[0xA6][2], rec['mode'])
            extra, plus = number(table('IX')[rec['opc']][2], rec['mode'])
            want = base + extra
        else:
            want, plus = number(table(rec['page'])[rec['opc']][2], rec['mode'])
            if rec['page'] != 'P00':
                want += 1               # the tables leave out the prefix's cycle
        seq = (table('P00')[0xA6] if rec['page'] == 'IX' else table(rec['page'])[rec['opc']])[4]
        if rec['mnemo'] in DIVIDE and ':fit' not in rec['key']:
            plus = True                 # overflowed: no count to hold it to
        if len({len(a) for a in seq.split('/')[0].split('@')}) > 1:
            # Alternatives that differ, which the matcher tries in either
            # mode: its call.
            plus = True
        if not plus and want != n:
            problems.append('table says %d cycles, the chip %d' % (want, n))
        return '%s %s' % (head, '; '.join(problems)) if problems else None

    ns.update(NAME=('MC6809', 'MC6809E'), ORG=ORG, FILL=FILL, BASE=BASE,
              fill_value=fill_value, table=table, patterns=patterns, cycles=cycles,
              record=record, span=span, bus_cycles=bus_cycles, restore=restore,
              check=check)


define(globals(), 'mc6809', native=False)
