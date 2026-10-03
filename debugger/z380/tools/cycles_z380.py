"""Z380 plugin for scripts/record-cycles.py.

Runs each pattern of libasm's gen_z380.lst (and every other opcode libasm
decodes) in isolation, memory filled with FF (RST 38H) so any transfer
breaks at once; derive_tables.py turns the recording into the
z380-PAGExx.txt tables. Needs Python 3.14 (compression.zstd) and the
profile image (-D PROFILE_CYCLES).

    P=debugger/z380/tools/cycles_z380.py
    R=debugger/z380/tools/z380-profile.jsonl.zst
    scripts/record-cycles.py $P fill                once per flash
    scripts/record-cycles.py $P run --record $R     every run not yet recorded
    scripts/record-cycles.py $P status --record $R
--only takes a run's key (page:opc[:variant]); --repeat N reruns every
tenth pattern N more times, as rep1..repN.

Z380_MODE=xm or Z380_MODE=lw records the patterns whose bus cycles
depend on the mode -- stack, transfers and memory operands -- again in
Extended or Long Word mode, keyed with an :xm or :lw suffix.
"""
import os
import re
from compression import zstd   # Python 3.14+

HERE = os.path.dirname(os.path.abspath(__file__))
MODE = os.environ.get('Z380_MODE', '')   # '', 'xm' or 'lw'
assert MODE in ('', 'xm', 'lw'), 'Z380_MODE is xm or lw'
GEN = os.path.join(HERE, 'gen_z380.lst.zst')   # libasm's, kept here
OPCODES = os.path.join(HERE, 'z380-opcodes.txt.zst')

NAME = 'Z380'

ORG = 0x100          # the pattern
EXIT = 0x38          # RST 38H: every FF the CPU runs into breaks here
PAD = 6              # FF after the pattern, over a longer predecessor
STACK = (0xC00, 0x1000)   # prefilled so that any pop yields 0020H
# An undefined opcode traps to 0000H, which holds FF too.

# The register base: address registers point into the FF fill. Set as
# 32-bit values, since (HL), (IX+d) and the like use the extensions in
# Native mode too.
BASE = [('PC', ORG), ('SP', 0xF00), ('HL', 0x2000), ('DE', 0x2100),
        ('IX', 0x2200), ('IY', 0x2300), ('BC', 0x2400), ('A', 0x55),
        ('F', 0), ('I', 0)]
BASE_MAP = dict(BASE)

# Reset after these: the next pattern must not inherit their state --
# a mode, a register selection, the interrupt state or an on-chip
# register (waits, IOCLK).
RESET_AFTER = {'HALT', 'RETN', 'RETI', 'IM', 'LDCTL', 'EI', 'DI', 'SETC',
               'RESC', 'EXX', 'EXALL', 'EXXX', 'EXXY', 'OUT0', 'OTIM',
               'OTIMR', 'OTDM', 'OTDMR', 'BTEST', 'MTEST'}
BLOCK_MEM = {'LDIR', 'LDDR', 'CPIR', 'CPDR'}
BLOCK_MEM_W = {'LDIRW', 'LDDRW'}
BLOCK_IO = {'INIR', 'INDR', 'OTIR', 'OTDR', 'INIRW', 'INDRW', 'OTIRW',
            'OTDRW'}
CONDS = ('NZ', 'Z', 'NC', 'C', 'PO', 'PE', 'P', 'M')


# Extended mode pops four bytes for a PC.
STACK_FILL = [0x20, 0x00, 0x00, 0x00] if MODE == 'xm' else [0x20, 0x00]


def baseline(addr):
    """What memory holds at |addr| when no pattern is loaded."""
    if STACK[0] <= addr < STACK[1]:
        return STACK_FILL[addr % len(STACK_FILL)]
    return 0xFF


# ------------------------------------------------------------ patterns
PREFIXES = {('00', 0xCB), ('00', 0xED), ('00', 0xDD), ('00', 0xFD),
            ('DD', 0xCB), ('FD', 0xCB), ('ED', 0xCB)}


def page_of(code):
    """(page, opcode) of an encoding; a DDIR decoder directive in front
    names its own page prefix, e.g. 'FDC2/00'."""
    ddir = ''
    wider = 0  # bytes an IB or IW directive adds to a displacement
    if len(code) > 2 and code[0] in (0xDD, 0xFD) and 0xC0 <= code[1] <= 0xC3:
        # DD: W, IB W, IW W, IB; FD: LW, IB LW, IW LW, IW
        wider = {(0xDD, 1): 1, (0xDD, 2): 2, (0xDD, 3): 1,
                 (0xFD, 1): 1, (0xFD, 2): 2, (0xFD, 3): 2}.get(
                (code[0], code[1] & 3), 0)
        ddir, code = '%02X%02X/' % (code[0], code[1]), code[2:]
    b0 = code[0]
    if b0 == 0xCB:
        page, opc = 'CB', code[1]
    elif b0 == 0xED:
        page, opc = ('EDCB', code[2]) if code[1] == 0xCB else ('ED', code[1])
    elif b0 in (0xDD, 0xFD):
        if code[1] == 0xCB:
            page, opc = '%02XCB' % b0, code[3 + wider]
        else:
            page, opc = '%02X' % b0, code[1]
    else:
        page, opc = '00', b0
    return ddir + page, opc


def _patterns():
    """Every pattern of gen_z380.lst, then every other opcode libasm decodes
    (z380-opcodes.txt.zst, one row per page and opcode with filler operand
    bytes): bytes, mnemonic, operands, page, opcode."""
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
    """gen_z380.lst's patterns. A long encoding continues its bytes on the
    next line, which has no mnemonic of its own."""
    out = []
    for line in zstd.open(GEN, 'rt'):
        m = re.match(r'^\s*[0-9A-F]+ : ((?:[0-9A-F]{2} ?){1,6})\s*(.*)$', line)
        if not m:
            continue
        code = bytes.fromhex(m.group(1).replace(' ', ''))
        text = m.group(2).split(None, 1)
        if not text:
            out[-1]['bytes'] += code.hex().upper()
            continue
        mnemo, operands = text[0], (text[1] if len(text) > 1 else '').strip()
        out.append(dict(index=len(out), bytes=code.hex().upper(), mnemo=mnemo,
                        operands=operands.replace(', ', ',')))
    for p in out:
        code = bytes.fromhex(p['bytes'])
        p['len'] = len(code)
        p['page'], p['opc'] = page_of(code)
    return out


def key(p, variant=''):
    k = '%s:%02X%s' % (p['page'], p['opc'], ':' + variant if variant else '')
    return k + ':' + MODE if MODE else k


# What a mode pass records again. Extended mode widens the PC and what the
# stack holds of it; Long Word mode widens register pairs moved through
# memory, the stack included.
TRANSFERS = {'CALL', 'CALR', 'RET', 'RETI', 'RETN', 'RST', 'JP', 'JR', 'DJNZ'}
PAIRS = re.compile(r'\b(BC|DE|HL|IX|IY|SP)\b')


def modal(p):
    if MODE == 'xm':
        return p['mnemo'] in TRANSFERS | {'PUSH', 'POP'} or '(SP' in p['operands']
    return p['mnemo'] in {'PUSH', 'POP'} or (
        '(' in p['operands'] and p['mnemo'] in {'LD', 'LDW', 'EX'}
        and PAIRS.search(p['operands'].replace('(SP', '(')) is not None)


def has_cond(p):
    head = p['operands'].split(',')[0].strip()
    return p['mnemo'] in ('JP', 'JR', 'CALL', 'CALR', 'RET') and head in CONDS


def patch_odd(p):
    """The pattern with its direct address, if any, made odd -- or None."""
    m = re.search(r'\(([0-9A-F]+)H\)', p['operands'])
    if not m:
        return None
    addr = int(m.group(1), 16)
    code = bytearray.fromhex(p['bytes'])
    lo, hi = addr & 0xFF, (addr >> 8) & 0xFF
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
    if has_cond(p):
        runs = [('f00', {'F': 0x00}), ('fff', {'F': 0xFF})]
    elif p['mnemo'] == 'DJNZ':
        runs = [('b1', {'BC': 0x0100}), ('b2', {'BC': 0x0200})]
    elif p['mnemo'] in BLOCK_MEM_W:
        # BC counts bytes, two per word: an odd count never reaches 0 and
        # the copy runs over all of memory, the trap at 0038H included.
        runs = [('c1', {'BC': 0x0002}), ('c2', {'BC': 0x0004})]
    elif p['mnemo'] in BLOCK_MEM:
        runs = [('c1', {'BC': 0x0001}), ('c2', {'BC': 0x0002})]
    elif p['mnemo'] in BLOCK_IO:
        # The word forms count bytes in BC, two per word.
        if p['mnemo'].endswith('W'):
            runs = [('c1', {'BC': 0x0002}), ('c2', {'BC': 0x0004})]
        else:
            runs = [('c1', {'BC': 0x0102}), ('c2', {'BC': 0x0202})]
    else:
        runs = [('', {})]
    out = [(name, over, p['bytes']) for name, over in runs]
    # A word at the other parity is a different transaction: run odd too.
    if '(' in p['operands'] and '(C)' not in p['operands'] \
            and '(SP)' not in p['operands']:
        odd_regs = {'HL': 0x2001, 'DE': 0x2101, 'IX': 0x2201, 'IY': 0x2301,
                    'BC': 0x2401}
        code = patch_odd(p) or p['bytes']
        for name, over in runs:
            if 'BC' in over:
                continue
            out.append((name + 'odd' if name else 'odd', dict(odd_regs, **over), code))
    return [(name, over, code, sp_source(p, over)) for name, over, code in out]


# ------------------------------------------------------------ console
# The profile build prints the slot and inject/capture flags first, then
# the strobes as S=, #BHEN and #BLEN as h= and l=, and the matcher's mark.
CYC = re.compile(r'^(?:([ic ]{0,2})\s*(\d+)\s+)?([RWIV]) ([AI])=([0-9A-F]{8}) '
                 r'D=([0-9A-F ]{4})(?: S=([0-9A-F]{2}) h=(\d) l=(\d))?')


def cycles(txt):
    """The ring dump in |txt| as [kind, addr, data, strobes, word] per cycle.

    kind is R/W for memory, r/w for I/O, a for an acknowledge; data is the
    word as printed, a lone byte on the lane the print put it on.
    """
    out = []
    for line in txt.splitlines():
        m = CYC.match(line)
        if not m:
            continue
        _, n, rw, ai, a, d, st, h, l = m.groups()
        if rw == 'I':
            rw = 'R'
        kind = 'a' if rw == 'V' else rw if ai == 'A' else rw.lower()
        word = int(h == '1' and l == '1') if h else -1
        out.append([kind, int(a, 16), d.replace(' ', '.'),
                    int(st, 16) if st else -1, word])
    return out


def swap(data):
    """A pushed word: the even byte rode D8-D15, printed first."""
    return int(data[2:4], 16) << 8 | int(data[0:2], 16)


def cut(cyc):
    """Split at the exit: (trace, exit_pushed, end).

    The debugger drops the ring from the vector fetch on, so a run that
    broke ends in the RST's push, whose data, byte-swapped, is the
    address after the FF that ran. A run that did not end in a write
    halted or was stopped.
    """
    # Extended mode pushes the PC as two words.
    if MODE == 'xm' and len(cyc) >= 2 and all(
            c[0] == 'W' and c[4] == 1 for c in cyc[-2:]):
        lo, hi = sorted(cyc[-2:], key=lambda c: c[1])
        if hi[1] == lo[1] + 2:
            return cyc[:-2], swap(hi[2]) << 16 | swap(lo[2]), 'exit'
    if cyc and cyc[-1][0] == 'W' and cyc[-1][4] == 1:
        return cyc[:-1], swap(cyc[-1][2]), 'exit'
    # SP odd: the push is two byte writes.
    if len(cyc) >= 2 and all(c[0] == 'W' and c[4] == 0 for c in cyc[-2:]):
        lo, hi = sorted(cyc[-2:], key=lambda c: c[1])
        if hi[1] == lo[1] + 1:
            byte = lambda c: int(c[2][2:4] if c[1] & 1 else c[2][0:2], 16)
            return cyc[:-2], byte(lo) | byte(hi) << 8, 'exit'
    return cyc, None, 'halt'


def far_targets():
    """Branch targets past 64K, which Extended mode reaches with DDIR
    immediates: the FF fill covers only the low 64K."""
    if MODE != 'xm':
        return []
    out = set()
    for p in _patterns():
        if p['mnemo'] not in TRANSFERS:
            continue
        m = re.search(r'\$([+-])([0-9A-F]+)H|\b0?([0-9A-F]{5,})H\b', p['operands'])
        if m:
            if m.group(2):
                disp = int(m.group(2), 16)
                addr = ORG + (disp if m.group(1) == '+' else -disp)
            else:
                addr = int(m.group(3), 16)
            if addr & 0xFFFFFF >= 0x10000:
                out.add(addr & 0xFFFFC0)
    return sorted(out)


FILL = [(0, 0x10000, [0xFF]),
        (STACK[0], (STACK[1] - STACK[0]) // len(STACK_FILL), STACK_FILL)] + \
       [(addr, 0x80, [0xFF]) for addr in far_targets()]


def patterns(args=None):
    """Every variant of every pattern, in the recording's order."""
    repeat = getattr(args, 'repeat', 0) or 0
    out = []
    for p in _patterns():
        if MODE and not modal(p):
            continue
        for variant, over, code, seed in variants(p):
            out.append(_run(p, variant, over, code, seed))
        if repeat and p['index'] % 10 == 0 and p['mnemo'] not in BLOCK_MEM | BLOCK_MEM_W | BLOCK_IO:
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
    # Taken on the next resume, with SETC XM or SETC LW.
    if MODE == 'xm':
        b.set_reg('XM', 1)
    elif MODE == 'lw':
        b.set_reg('LW', 1)
    b.set_regs(BASE)


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
    if not trace or trace[0][0] != 'R' or (trace[0][1] & ~1) != ORG:
        anomaly.append('leading')
    if end == 'halt' and run['mnemo'] not in ('HALT', 'SLP'):
        anomaly.append('nohalt')
    # Prefetch past the pattern: overall, and before its last data cycle.
    end_addr = ORG + run['len']
    is_fetch = lambda c: c[0] == 'R' and ORG <= c[1] < end_addr + 16
    last_data = max((i for i, c in enumerate(trace) if not is_fetch(c)), default=-1)
    ahead = lambda cs: max((c[1] for c in cs if is_fetch(c) and c[1] >= end_addr),
                           default=end_addr - 1) - (end_addr - 1)
    rec['ahead_at_exit'] = ahead(trace)
    rec['ahead_before_data'] = ahead(trace[:last_data + 1])
    rec['anomaly'] = ','.join(anomaly)
    return rec


def restore(run, cyc):
    """Put back what the run wrote, and the seed, word by word; the pattern
    area is rewritten by the next run."""
    seed = run['seed']
    dirty = sorted({c[1] & 0xFFFE for c in cyc if c[0] == 'W'} |
                   ({seed[0] & ~1, (seed[0] + 1) & ~1} if seed else set()))
    return [(addr, [baseline(addr), baseline(addr + 1)]) for addr in dirty
            if not ORG <= addr < ORG + 16]


def after(b, rec):
    # POP SR loads the modes and register selections from the stack fill.
    if rec['mnemo'] in RESET_AFTER or rec['end'] in ('halt', 'timeout') \
            or (rec['mnemo'] == 'EX' and "AF'" in rec['operands']) \
            or (rec['mnemo'] == 'POP' and rec['operands'] == 'SR'):
        reset(b)


def check(rec):
    """The tables come from derive_tables.py; here only what went wrong."""
    if rec['end'] == 'timeout':
        return 'timeout'
    return rec.get('anomaly') or None
