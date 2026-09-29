"""Z280 plugin for scripts/record-cycles.py.

Runs each pattern of libasm's gen_z280.lst (and every other opcode libasm
decodes) in isolation, memory filled with FF (RST 38H) so any transfer
breaks at once; derive_tables.py turns the recording into the
z280-PAGExx.txt tables. Needs Python 3.14 (compression.zstd) and
the profile image (-D PROFILE_CYCLES).

    P=debugger/z280/tools/cycles_z280.py
    R=debugger/z280/tools/z280-profile.jsonl.zst
    scripts/record-cycles.py $P fill                once per flash
    scripts/record-cycles.py $P run --record $R     every run not yet recorded
    scripts/record-cycles.py $P status --record $R
--only takes a run's key (page:opc[:variant]); --repeat N reruns every
tenth pattern N more times, as rep1..repN.
"""
import os
import re
from compression import zstd   # Python 3.14+

HERE = os.path.dirname(os.path.abspath(__file__))
GEN = os.path.join(HERE, 'gen_z280.lst.zst')   # libasm's, kept here

NAME = 'Z280'

ORG = 0x100          # the pattern; gen_z280.lst assembles at 0100H too
EXIT = 0x38          # RST 38H: every FF the CPU runs into breaks here
PAD = 6              # FF after the pattern, over a longer predecessor
STACK = (0xC00, 0x1000)   # prefilled 20 00: any pop yields 0020H
IVT = (0x1000, 0x1400)    # {MSR=0000, PC=0040}: any trap breaks at 0040H
IVTP = IVT[0] >> 8        # A23-A12 of the table, in bits 15-4

# The register base: address registers point into the FF fill.
BASE = [('PC', ORG), ('SP', 0xF00), ('USP', 0xD00), ('HL', 0x2000),
        ('DE', 0x2100), ('IX', 0x2200), ('IY', 0x2300), ('BC', 0x2400),
        ('A', 0x55), ('F', 0), ('I', 0), ('MSR', 0), ('IOP', 0)]
BASE_MAP = dict(BASE)

# Reset after these: the next pattern must not inherit their state.
RESET_AFTER = {'HALT', 'RETIL', 'RETN', 'RETI', 'SC', 'IM', 'LDCTL', 'EI',
               'DI', 'LDUD', 'LDUP', 'EPUM', 'MEPU', 'EPUF', 'EPUI'}
BLOCK_MEM = {'LDIR', 'LDDR', 'CPIR', 'CPDR'}
BLOCK_IO = {'INIR', 'INDR', 'OTIR', 'OTDR', 'INIRW', 'INDRW', 'OTIRW', 'OTDRW'}
CONDS = ('NZ', 'Z', 'NC', 'C', 'PO', 'PE', 'P', 'M')


def baseline(addr):
    """What memory holds at |addr| when no pattern is loaded."""
    if STACK[0] <= addr < STACK[1]:
        return (0x20, 0x00)[addr & 1]
    if IVT[0] <= addr < IVT[1]:
        return (0x00, 0x00, 0x40, 0x00)[addr & 3]
    return 0xFF


# ------------------------------------------------------------ patterns
OPCODES = os.path.join(HERE, 'z280-opcodes.txt.zst')
PREFIXES = {('00', 0xCB), ('00', 0xED), ('00', 0xDD), ('00', 0xFD),
            ('DD', 0xCB), ('DD', 0xED), ('FD', 0xCB), ('FD', 0xED)}


def _patterns():
    """Every pattern of gen_z280.lst, then every other opcode libasm decodes
    (profile/z280-opcodes.txt.gz, one row per page and opcode with filler
    operand bytes): bytes, mnemonic, operands, page, opcode."""
    out = gen_patterns()
    seen = {(p['page'], p['opc']) for p in out}
    for line in zstd.open(OPCODES, 'rt'):
        page, opc, length, code, mnemo, operands = (line.rstrip('\n').split(' ', 5) + [''])[:6]
        opc = int(opc, 16)
        if not int(length) or (page, opc) in seen or (page, opc) in PREFIXES:
            continue
        out.append(dict(index=len(out), bytes=code, len=int(length), page=page,
                        opc=opc, mnemo=mnemo, operands=operands.strip().replace(', ', ',')))
    return out


def gen_patterns():
    out = []
    for line in zstd.open(GEN, 'rt'):
        m = re.match(r'^\s*([0-9A-F]+) : ((?:[0-9A-F]{2} )+)\s*(\S+)\s*(.*)$', line)
        if not m:
            continue
        code = bytes.fromhex(m.group(2).replace(' ', ''))
        mnemo, operands = m.group(3), m.group(4).strip()
        if mnemo in ('CPU', 'ORG'):
            continue
        b0 = code[0]
        if b0 in (0xCB, 0xED):
            page, opc = '%02X' % b0, code[1]
        elif b0 in (0xDD, 0xFD):
            if code[1] == 0xCB:
                page, opc = '%02XCB' % b0, code[3]
            elif code[1] == 0xED:
                page, opc = '%02XED' % b0, code[2]
            else:
                page, opc = '%02X' % b0, code[1]
        else:
            page, opc = '00', b0
        out.append(dict(index=len(out), bytes=code.hex().upper(), len=len(code),
                        page=page, opc=opc, mnemo=mnemo, operands=operands))
    return out


def key(p, variant=''):
    return '%s:%02X%s' % (p['page'], p['opc'], ':' + variant if variant else '')


def has_cond(p):
    ops = p['operands']
    head = ops.split(',')[0].strip()
    return p['mnemo'] in ('JP', 'JR', 'CALL', 'RET') and head in CONDS


def patch_odd(p):
    """The pattern with its direct address, if any, made odd -- or None."""
    m = re.search(r'\(([0-9A-F]+)H\)', p['operands'])
    if not m:
        return None
    addr = int(m.group(1), 16)
    code = bytearray.fromhex(p['bytes'])
    lo, hi = addr & 0xFF, addr >> 8
    for i in range(len(code) - 1):
        if code[i] == lo and code[i + 1] == hi:
            code[i] ^= 1
            return code.hex().upper()
    return None


def sp_source(p, regs):
    """Where a load of SP from memory reads: (addr, bytes) to seed, or None.

    The FF fill would load SP with FFFFH, and with SP odd the exit's push
    is two byte writes the debugger does not take for a break.
    """
    if p['mnemo'] not in ('LD', 'LDW') or not p['operands'].startswith('SP,'):
        return None
    m = re.search(r'\((HL|IX|IY)([+-]\d+)?\)', p['operands'])
    if m:
        addr = regs.get(m.group(1), BASE_MAP[m.group(1)]) + int(m.group(2) or 0)
    else:
        m = re.search(r'\(([0-9A-F]+)H\)', p['operands'])
        if not m:
            return None
        addr = int(m.group(1), 16)
        if 'HL' in regs:          # the odd variant patched the address
            addr ^= 1
    return addr & 0xFFFF, [BASE_MAP['SP'] & 0xFF, BASE_MAP['SP'] >> 8]


def variants(p):
    """(variant name, register overrides, bytes, memory seed) per run."""
    regs = {}
    runs = []
    if has_cond(p):
        runs = [('f00', {'F': 0x00}), ('fff', {'F': 0xFF})]
    elif p['mnemo'] == 'DJNZ':
        runs = [('b1', {'BC': 0x0100}), ('b2', {'BC': 0x0200})]
    elif p['mnemo'] in BLOCK_MEM:
        runs = [('c1', {'BC': 0x0001}), ('c2', {'BC': 0x0002})]
    elif p['mnemo'] in BLOCK_IO:
        runs = [('c1', {'BC': 0x0102}), ('c2', {'BC': 0x0202})]
    elif p['mnemo'] == 'LDCTL' and '(C)' in p['operands']:
        runs = [('', {'BC': 0x0004})]     # the Stack Limit register
    else:
        runs = [('', {})]
    out = [(name, dict(regs, **over), p['bytes'], None) for name, over in runs]
    # A word at the other parity is a different transaction: run odd too.
    if '(' in p['operands'] and '(C)' not in p['operands'] \
            and '(SP)' not in p['operands']:
        odd_regs = {'HL': 0x2001, 'DE': 0x2101, 'IX': 0x2201, 'IY': 0x2301,
                    'BC': 0x2401}
        code = patch_odd(p) or p['bytes']
        for name, over in runs:
            if 'BC' in over:
                continue
            out.append((name + 'odd' if name else 'odd',
                        dict(odd_regs, **over), code, None))
    return [(name, over, code, sp_source(p, over)) for name, over, code, _ in out]


# ------------------------------------------------------------ console
# A matched fetch prints as I; slot and flags only from a profiling build.


# ------------------------------------------------------------ console
# A matched fetch prints as I; slot and flags only from a profiling build.
CYC = re.compile(r'^(?:([ic ]{0,2})\s*(\d+)\s+)?([RWI]) ([AI])=([0-9A-F]{6}) '
                 r'D=([0-9A-F ]{4})(?: S=([0-9A-F]) b=(\d) r=(\d))?')



def cycles(txt):
    """The ring dump in |txt| as [kind, addr, data, st, bw] per cycle.

    kind is R/W for memory, r/w for I/O; data is the AD word as printed,
    with a lone byte at the lane the print put it on.
    """
    out = []
    for line in txt.splitlines():
        m = CYC.match(line)
        if not m:
            continue
        _, n, rw, ai, a, d, st, bw, _ = m.groups()
        if rw == 'I':
            rw = 'R'
        kind = rw if ai == 'A' else rw.lower()
        out.append([kind, int(a, 16), d.replace(' ', '.'),
                    int(st, 16) if st else -1, int(bw) if bw else -1])
    return out


def swap(data):
    """A pushed word: the even byte rode AD8-15, printed first."""
    return int(data[2:4], 16) << 8 | int(data[0:2], 16)


def cut(cyc):
    """Split at the exit: (trace, exit_pushed, end).

    The debugger drops the ring from the vector fetch on (the RET and JP
    that unwind the restart), so a run that broke ends in the RST's
    push, whose data, byte-swapped, is the address after the FF that
    ran. A run that did not end in a write halted or was stopped.
    """
    if cyc and cyc[-1][0] == 'W' and cyc[-1][4] == 0:
        return cyc[:-1], swap(cyc[-1][2]), 'exit'
    # SP odd: the push is two byte writes.
    if len(cyc) >= 2 and all(c[0] == 'W' and c[4] == 1 for c in cyc[-2:]):
        lo, hi = sorted(cyc[-2:], key=lambda c: c[1])
        if hi[1] == lo[1] + 1:
            byte = lambda c: int(c[2][2:4] if c[1] & 1 else c[2][0:2], 16)
            return cyc[:-2], byte(lo) | byte(hi) << 8, 'exit'
    return cyc, None, 'halt'


FILL = [(0, 0x10000, [0xFF]),
        (STACK[0], (STACK[1] - STACK[0]) // 2, [0x20, 0x00]),
        (IVT[0], (IVT[1] - IVT[0]) // 4, [0x00, 0x00, 0x40, 0x00])]


def patterns(args=None):
    """Every variant of every pattern, in the recording's order."""
    repeat = getattr(args, 'repeat', 0) or 0
    out = []
    for p in _patterns():
        for variant, over, code, seed in variants(p):
            out.append(_run(p, variant, over, code, seed))
        if repeat and p['index'] % 10 == 0 and p['mnemo'] not in BLOCK_MEM | BLOCK_IO:
            for i in range(1, repeat + 1):
                out.append(_run(p, 'rep%d' % i, {}, p['bytes'], None))
    return out


def _run(p, variant, over, code, seed):
    data = list(bytes.fromhex(code)) + [0xFF] * PAD
    return dict(key=key(p, variant), index=p['index'], code=code, len=p['len'],
                page=p['page'], opc=p['opc'], mnemo=p['mnemo'],
                operands=p['operands'], variant=variant, regs=over,
                bytes=data[:16], seed=seed)


def schedule(runs):
    """The disruptive ones last."""
    return sorted(runs, key=lambda r: (r['mnemo'] in RESET_AFTER, r['index']))


def setup(b):
    """Verbose on, so a run prints its cycles; then a clean CPU."""
    b.abort()
    b.until_prompt(30.0)
    if 'Verbose OFF' in b.cmd('V'):
        b.cmd('V')
    reset(b)


def reset(b):
    b.cmd('R', 30.0)
    b.set_regs(BASE)
    # IVTP after a reset is unknown: set it, like any pattern.
    b.write(ORG, [0x0E, 0x06, 0x21, IVTP & 0xFF, IVTP >> 8, 0xED, 0x6E] + [0xFF] * 5)
    b.run()


def record(run, cyc, ok):
    regs = dict(BASE_MAP, **run['regs'])
    rec = dict(key=run['key'], index=run['index'], bytes=run['code'], len=run['len'],
               page=run['page'], opc=run['opc'], mnemo=run['mnemo'],
               operands=run['operands'], variant=run['variant'], regs=regs)
    if not ok:
        rec.update(end='timeout', cycles=[])
        return rec
    trace, pushed, end = cut(cyc)
    rec.update(end=end, cycles=trace, exit_pushed=pushed)
    anomaly = []
    if not trace or trace[0][0] != 'R' or trace[0][1] != ORG:
        anomaly.append('leading')
    if end == 'halt' and run['mnemo'] != 'HALT':
        anomaly.append('nohalt')
    # Prefetch past the pattern: overall, and before its last data cycle.
    end_addr = ORG + run['len']
    is_fetch = lambda c: c[0] == 'R' and ORG <= c[1] < end_addr + 16
    last_data = max((i for i, c in enumerate(trace) if not is_fetch(c)), default=-1)
    ahead = lambda cs: max((c[1] for c in cs if is_fetch(c) and c[1] >= end_addr), default=end_addr - 1) - (end_addr - 1)
    rec['ahead_at_exit'] = ahead(trace)
    rec['ahead_before_data'] = ahead(trace[:last_data + 1])
    rec['anomaly'] = ','.join(anomaly)
    return rec


def restore(run, cyc):
    """Put back what the run wrote, and the seed, word by word; the pattern
    area is rewritten by the next run."""
    seed = run['seed']
    dirty = sorted({c[1] & ~1 for c in cyc if c[0] == 'W'} |
                   ({seed[0] & ~1, (seed[0] + 1) & ~1} if seed else set()))
    return [(addr, [baseline(addr), baseline(addr + 1)]) for addr in dirty
            if not ORG <= addr < ORG + 16]


def after(b, rec):
    if rec['mnemo'] in RESET_AFTER or rec['end'] in ('halt', 'timeout'):
        reset(b)


def check(rec):
    """The tables come from derive_tables.py; here only what went wrong."""
    if rec['end'] == 'timeout':
        return 'timeout'
    return rec.get('anomaly') or None
