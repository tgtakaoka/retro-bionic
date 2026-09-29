"""SCN2650 plugin for scripts/record-cycles.py.

Each pattern is one opcode of scn2650.txt, direct and indirect where it
has an I bit, both ways for a conditional branch or return, and the index
modes for absolute operands, run from ORG in memory filled with HALT. A
transfer lands in the fill and a pointer read from it is 4040, the fill
again, so every pattern ends at a HALT. Needs the profile image
(-D PROFILE_CYCLES), whose run loop clocks the chip, without consulting
the table, until a HALT puts it in the WAIT state, so the last cycle is
the HALT's fetch, and then resets it. Its dump marks the matcher's fetches
with the cycles it gave them (m=), and every cycle with the clock periods
OPREQ stayed low before it (c=): one processor cycle per OPREQ and three
clock periods per processor cycle without one. check() holds both against
the table's bus cycles and processor cycles.

    P=debugger/scn2650/tools/cycles_scn2650.py
    R=debugger/scn2650/tools/scn2650-cycles.jsonl.zst
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import os
import re

ARCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

NAME = 'SCN2650'
HALT = 0x40
ORG = 0x1000
MEMORY = 0x8000
FILL = [(0, MEMORY, [HALT])]
# The run loop's bound; a run this long never reached a HALT.
MAX_CYCLES = 96
# Bus cycles the chip makes after a HALT's fetch before WAIT: none.
HALT_TAIL = 0

# PSU's II keeps interrupts off and SP at 0; PSL keeps RS at bank 0, and
# its CC is what the conditional patterns set. R3 indexes, and R0 is also
# what LPSU and LPSL load.
R3 = 0x10
BASE = [('PSL', 0x00), ('PSU', 0x20), ('R0', 0x20), ('R1', 0x11),
        ('R2', 0x22), ('R3', R3), ('PC', ORG)]

REL = 0x20                      # +32 from the byte after the instruction
DATA = 0x1C00                   # absolute operands, ORG's page
TARGET = 0x1100                 # absolute branches
POINTER = (HALT << 8 | HALT) & 0x7FFF     # an indirect address read from the fill
PORT = 0x80                     # REDE/WRTE: clear of the USART at 00
IMM = 0x00                      # immediates; changes no PSU/PSL bit
INDIRECT = 0x80
INDEX = {'xi': 1, 'xd': 2, 'x': 3}   # absolute operand bits 6-5
PAD = 4      # every write is this long, so no longer earlier pattern lingers
ARITH = re.compile(r'^(LOD|EOR|AND|IOR|ADD|SUB|STR|COM)([ZIRA])$')
ON_CC = re.compile(r'^(BCT|BST|BCF|BSF)[RA]$|^RET[CE]$')
ON_REG = re.compile(r'^(BRN|BSN|BIR|BDR)[RA]$')


def table():
    """{opc: (mnemonic, operands, length, cycles, bus, indirect)}."""
    out = {}
    for line in open(os.path.join(ARCH, 'scn2650.txt')):
        f = line.split()
        if len(f) == 7 and re.fullmatch(r'[0-9A-F]{2}', f[0]) and f[1] != '-':
            out[int(f[0], 16)] = (f[1], f[2], int(f[3]), int(f[4]), int(f[5]),
                                  f[6] == '1')
    return out


def _taken(mnemo, opc, regs):
    """Whether a conditional transfer takes its branch with |regs|."""
    v = opc & 3
    if ON_CC.match(mnemo):
        cc = regs['PSL'] >> 6
        if mnemo[:3] in ('BCF', 'BSF'):
            return v != cc
        return v == 3 or v == cc
    r = regs['R%d' % v]
    if mnemo.startswith('BIR'):
        return (r + 1) & 0xFF != 0
    if mnemo.startswith('BDR'):
        return (r - 1) & 0xFF != 0
    return r != 0


def _variants(mnemo, opc):
    """[(suffix, regs)] for each way a conditional instruction can go."""
    v = opc & 3
    if ON_CC.match(mnemo) and not (v == 3 and mnemo[:3] in ('BCT', 'BST', 'RET')):
        # CC matching the v field, then not.
        return [(':cc%d' % cc, {'PSL': cc << 6}) for cc in (v, (v + 1) % 3)]
    if ON_REG.match(mnemo):
        # Taken, then not.
        values = {'BIR': (0x00, 0xFF), 'BDR': (0x00, 0x01)}.get(mnemo[:3], (0x01, 0x00))
        return [(':r%02x' % r, {'R%d' % v: r}) for r in values]
    return [('', {})]


def _operands(mnemo, opr, length, indirect, index=None):
    """The operand bytes, and where a taken transfer goes."""
    i = INDIRECT if indirect else 0
    if length == 1:
        return [], None
    if opr.endswith('io'):
        return [PORT], None
    if opr.endswith('nn'):
        return [IMM], None
    if opr.endswith('rel'):
        if ARITH.match(mnemo):      # the data's address; no transfer
            return [i | REL], None
        if mnemo in ('ZBRR', 'ZBSR'):
            return [i | REL], POINTER if indirect else REL
        return [i | REL], POINTER if indirect else ORG + 2 + REL
    if ARITH.match(mnemo):
        x = INDEX[index] << 5 if index else 0
        return [i | x | DATA >> 8 & 0x1F, DATA & 0xFF], None
    target = POINTER if indirect else TARGET
    if mnemo in ('BXA', 'BSXA'):
        target += R3
    return [i | TARGET >> 8, TARGET & 0xFF], target


def patterns(args=None):
    out = []
    base = dict(BASE)
    for opc, (mnemo, opr, length, cyc, bus, indir) in sorted(table().items()):
        if opc == HALT:
            continue
        modes = [(False, None)]
        if indir:
            modes.append((True, None))
        if ARITH.match(mnemo) and mnemo.endswith('A') and opc & 3 == 3:
            modes += [(False, x) for x in sorted(INDEX)]
        for indirect, index in modes:
            code, target = _operands(mnemo, opr, length, indirect, index)
            key = '%02X' % opc + (':i' if indirect else '') + (':' + index if index else '')
            for suffix, regs in _variants(mnemo, opc):
                run = dict(key=key + suffix, mnemo=mnemo, operands=opr,
                           indirect=indirect,
                           bytes=[opc] + code + [HALT] * (PAD - 1 - len(code)))
                if regs:
                    run['regs'] = regs
                nxt = ORG + length
                if target is not None and (not suffix or _taken(mnemo, opc, dict(base, **regs))):
                    nxt = target
                if mnemo.startswith('RET') and _taken(mnemo, opc, dict(base, **regs)):
                    nxt = None      # wherever the return stack points
                run['next'] = nxt
                out.append(run)
    return out


def cycles(txt):
    """[kind, addr, data, matched, clocks] per printed cycle; kind is M or
    I (I/O) and R or W, addr a number for memory, the printed port for
    I/O."""
    out = []
    for m in re.finditer(r'^([MIV][RW]) A=(\S+) D=([0-9A-F]{2}) m=(\d+) c=(\d+)$',
                         txt, re.M):
        addr = int(m.group(2), 16) if m.group(1)[0] == 'M' else m.group(2)
        out.append([m.group(1), addr, int(m.group(3), 16), int(m.group(4)),
                    int(m.group(5))])
    return out


def record(run, cycles, ok):
    end = 'timeout' if not ok else 'cycles' if len(cycles) >= MAX_CYCLES else 'trap'
    return dict(run, end=end, cycles=cycles)


def restore(run, cycles):
    return [(addr, [HALT]) for addr in sorted({c[1] for c in cycles if c[0] == 'MW'})]


def after(b, rec):
    """The profile image resets the CPU after every run."""


def check(rec):
    mnemo, opr, length, cyc, bus, indir = table()[int(rec['key'][:2], 16)]
    head = '%-4s %-7s' % (mnemo, opr)
    trace = rec['cycles']
    if rec['end'] != 'trap':
        return '%s ended %s after %d cycles' % (head, rec['end'], len(trace))
    # The chip's: the HALT's fetch, the last cycle but its tail.
    t = len(trace) - 1 - HALT_TAIL
    # A branch not taken reads no indirect address.
    skipped = mnemo.startswith('B') and rec['next'] == ORG + length
    extra = 2 if rec['indirect'] and not skipped else 0
    problems = []
    if t < 1 or trace[0][:2] != ['MR', ORG]:
        return '%s did not start at %04X' % (head, ORG)
    trap = trace[t]
    if trap[0] != 'MR' or trap[2] != HALT:
        problems.append('ended at %s %s D=%02X, not a HALT' % (trap[0], trap[1], trap[2]))
    elif rec['next'] is not None and trap[1] != rec['next']:
        problems.append('went to %04X, not %04X' % (trap[1], rec['next']))
    marks = [i for i, c in enumerate(trace) if c[3]]
    if marks != [0, t] or trace[0][3] != t:
        problems.append('matcher marked %s, the chip fetched at 0 and %d'
                        % (' '.join('%d(m=%d)' % (i, trace[i][3]) for i in marks), t))
    if length + bus + extra != t:
        problems.append('table says %d bus cycles, the chip %d' % (length + bus + extra, t))
    # Three clock periods per processor cycle without OPREQ.
    clocks = [c[4] for c in trace[1:t + 1]]
    if any(c % 3 != 1 for c in clocks):
        problems.append('OPREQ waits of %s clocks' % ' '.join(map(str, clocks)))
    else:
        took = sum((c + 2) // 3 for c in clocks)
        if cyc + extra != took:
            problems.append('table says %d processor cycles, the chip %d'
                            % (cyc + extra, took))
    return '%s %s' % (head, '; '.join(problems)) if problems else None
