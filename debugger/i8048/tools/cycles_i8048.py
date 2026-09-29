"""MCS-48 plugin for scripts/record-cycles.py, for an Intel P8039/P8048 on
the P8048 board; cycles_msm80c39.py is the same for the OKI parts.

Each pattern is one opcode of i8048.txt with HALT ($01) as its operand
byte, run from ORG in program memory filled with HALT, so any transfer
stops at once. The debugger stops at any HALT or undefined opcode it is
about to fetch, so HALT is the trap on the Intel parts too. Needs the
profile image (-D PROFILE_CYCLES), whose dump prints every strobed cycle
(P #PSEN, R/W #RD/#WR, a port letter for PROG) with m=, the bus cycles
the matcher gave an opcode fetch, and i=, the machine cycles with no
strobe before it. The trap's fetch shows the JMP the debugger injects in
its place. check() holds the matcher's marks and counts against the run,
and the run's machine cycles against the table.

    P=debugger/i8048/tools/cycles_i8048.py
    R=debugger/i8048/tools/i8048-cycles.jsonl.zst
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# i8048.txt's flags.
F_I8039 = 1
F_80C39 = 2
F_MSM39 = 4
F_X8048 = 8


def define(ns, boards, flags):
    """Fill |ns| with the plugin for the opcodes of |flags| on |boards|."""
    TRAP = 0x01                 # HALT
    ORG = 0x100
    PAD = 4
    PTR = 0x20                  # R0, R1, R3: internal RAM above the stack,
                                # external RAM clear of the USART at FC
    # PSW: SP 0, register bank 0, flags clear; A high keeps the ports up.
    BASE = [('PC', ORG), ('MB', 0), ('PSW', 0x08), ('A', 0xFF), ('F1', 0),
            ('R0', PTR), ('R1', PTR), ('R3', PTR)]
    FILL = [(0x000, 0x1000, [TRAP])]
    MAX_CYCLES = 96
    FRES = 0xE2
    # Flag conditions, run once clear and once set.
    FLAGGED = re.compile(r'^(JB|JC|JNC|JF0|JF1|JZ|JNZ)$')
    # Opcodes that leave the CPU in a state a later run would see: interrupt
    # and timer enables, the T0 clock and the port latches. A reset after
    # them puts that back.
    RESET_AFTER = {0x05, 0x25, 0x45, 0x55, 0x75, 0x39, 0x3A, 0x89, 0x8A,
                   0x99, 0x9A, 0xA2, 0xC2, 0xC3, 0xF3} | \
        set(range(0x3C, 0x40)) | set(range(0x8C, 0x90)) | set(range(0x9C, 0xA0))
    # Not run: HLTS stops the clock until a reset. The BUS forms (flag 8)
    # are left out by |flags|: the debugger takes them for undefined, as
    # the external program memory needs BUS, and would take OUTL BUS's
    # #WR for an external RAM write.
    # FLTT runs with FRES after it, as FLT does.
    SKIP = {TRAP, 0x82}

    def table():
        """{opc: (mnemonic, operands, length, cycles)} for |flags|."""
        out = {}
        for line in open(os.path.join(ARCH, 'i8048.txt')):
            f = line.split()
            if len(f) == 7 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[1] != '-':
                if int(f[5]) & flags:
                    out[int(f[0], 16)] = (f[1], f[2], int(f[3]), int(f[4]))
        return out

    def patterns(args=None):
        out = []
        for opc, (mnemo, opr, length, cyc) in sorted(table().items()):
            if opc in SKIP:
                continue
            code = [opc] + [TRAP] * (length - 1)
            starts = [0]                # where the pattern's opcodes are
            if mnemo in ('FLT', 'FLTT'):
                starts.append(len(code))
                code.append(FRES)       # FLT floats P1/P2, FLTT the strobes
            base = dict(key='%02X' % opc, mnemo=mnemo, operands=opr, opc=opc,
                        starts=starts, bytes=code + [TRAP] * (PAD - len(code)))
            if mnemo in ('RET', 'RETR'):
                # SP 1 pops stack slot 0 (08-09): return to 200, in the fill.
                base.update(regs={'PSW': 0x09}, data_seed=(0x08, [0x00, 0x02]))
            if FLAGGED.match(mnemo):
                out.append(dict(base, key=base['key'] + ':0',
                                regs={'PSW': 0x08, 'A': 0x00, 'F1': 0}))
                out.append(dict(base, key=base['key'] + ':1',
                                regs={'PSW': 0xE8, 'A': 0xFF, 'F1': 1}))
            elif mnemo == 'DJNZ' and opr.startswith('R'):
                r = opr.split(',')[0]
                out.append(dict(base, key=base['key'] + ':0', regs={r: 1}))
                out.append(dict(base, key=base['key'] + ':1', regs={r: 2}))
            else:
                out.append(base)
        return out

    def cycles(txt):
        """[kind, addr, data, matched, idles] per printed cycle; kind is F
        for #PSEN, R or W for #RD or #WR, X for PROG."""
        out = []
        for m in re.finditer(r'^([PRWOA ]) ([AXP])[= ]([0-9A-F ]{3,4}) D=([0-9A-F]{2})'
                             r' m=(\d+) i=(\d+)', txt, re.M):
            kind = {'A': 'F', 'P': 'X'}.get(m.group(2), m.group(1))
            out.append([kind, int(m.group(3).strip(), 16), int(m.group(4), 16),
                        int(m.group(5)), int(m.group(6))])
        return out

    def holds_trap(run, addr):
        code = run['bytes']
        return not ORG <= addr < ORG + len(code) or code[addr - ORG] == TRAP

    def record(run, cycles, ok):
        """The run up to and including the trap's fetch, its last cycle."""
        rec = dict(run, end='timeout' if not ok else 'other', cycles=cycles)
        if ok and cycles and len(cycles) < MAX_CYCLES:
            last = cycles[-1]
            if last[0] == 'F' and last[3] and holds_trap(run, last[1]):
                rec['end'] = 'trap'
        return rec

    def restore(run, cycles):
        # Runs write only internal and external data memory, which no
        # pattern reads back.
        return []

    def after(b, rec):
        if rec['end'] != 'trap' or rec['opc'] in RESET_AFTER:
            b.cmd('R', 30.0)

    def check(rec):
        tab = table()
        mnemo, opr, length, cyc = tab[rec['opc']]
        head = '%-5s %-8s' % (mnemo, opr)
        trace = rec['cycles']
        if rec['end'] != 'trap':
            return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
        # The pattern's opcodes, then the trap's.
        marks = [i for i, c in enumerate(trace) if c[3]]
        expect = [ORG + i for i in rec['starts']]
        if [trace[i][1] for i in marks[:-1]] != expect or marks[0] != 0 \
                or marks[-1] != len(trace) - 1:
            return '%s matcher marked fetches at %s' % (
                head, ' '.join('%03X' % trace[i][1] for i in marks))
        problems = []
        for i, j in zip(marks, marks[1:]):
            opc = rec['bytes'][trace[i][1] - ORG]
            strobed = j - i
            machine = strobed + sum(c[4] for c in trace[i + 1:j + 1])
            if trace[i][3] != strobed:
                problems.append('%02X: matcher took %d cycles, the chip strobed %d'
                                % (opc, trace[i][3], strobed))
            if tab[opc][3] != machine:
                problems.append('%02X: table says %d cycles, the chip %d'
                                % (opc, tab[opc][3], machine))
        return '%s %s' % (head, '; '.join(problems)) if problems else None

    ns.update(NAME=boards, ORG=ORG, FILL=FILL, BASE=BASE, table=table,
              patterns=patterns, cycles=cycles, record=record,
              restore=restore, after=after, check=check)


define(globals(), ('P8039',), F_I8039)
