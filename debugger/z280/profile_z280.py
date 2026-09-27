#!/usr/bin/env python3
"""Record the bus cycles of every Z280 instruction pattern, on the bench.

Runs each pattern of libasm's gen_z280.lst (and every other opcode libasm
decodes) in isolation, memory filled with FF (RST 38H) so any transfer
breaks at once, and appends one JSON line per run to
profile/z280-profile.jsonl.zst; derive_z280.py turns it into the
z280-PAGExx.txt tables. Needs Python 3.14 (compression.zstd) and the
firmware built with -D Z280_PROFILE.

    profile_z280.py fill            fill memory, once per flash
    profile_z280.py run [opts]      record every pattern not yet recorded
    profile_z280.py status          what is recorded, what is not
    profile_z280.py check           run the samples (a normal build) and
                                    compare every disassembled line of
                                    the dump with samples/z280/*.lst
      --only MNEMO   only patterns with this mnemonic
      --redo KEY     rerun one key (page:opc[:variant]) even if recorded
      --from N       start at pattern index N
      --limit N      stop after N runs
      --repeat N     rerun every tenth pattern N more times, as rep1..repN
"""
import argparse
import importlib.machinery
import json
import os
import re
import sys
import time
from compression import zstd   # Python 3.14+

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.dirname(os.path.dirname(HERE))
GEN = os.path.join(HERE, 'profile', 'gen_z280.lst.zst')   # libasm's, kept here
OUT_DIR = os.path.join(HERE, 'profile')
OUT = os.path.join(OUT_DIR, 'z280-profile.jsonl.zst')

bc = importlib.machinery.SourceFileLoader(
    'bc', os.path.join(PROJ, 'scripts', 'bionic-control.py')).load_module()

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
OPCODES = os.path.join(HERE, 'profile', 'z280-opcodes.txt.zst')
PREFIXES = {('00', 0xCB), ('00', 0xED), ('00', 0xDD), ('00', 0xFD),
            ('DD', 0xCB), ('DD', 0xED), ('FD', 0xCB), ('FD', 0xED)}


def patterns():
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
CYC = re.compile(r'^(?:([ic ]{0,2})\s*(\d+)\s+)?([RWI]) ([AI])=([0-9A-F]{6}) '
                 r'D=([0-9A-F ]{4})(?: S=([0-9A-F]) b=(\d) r=(\d))?')


def t(raw):
    return raw.decode('ascii', 'replace').replace('\r', '')


class Board:
    def __init__(self):
        self.fd, who = bc.open_board()
        if who != 'Z280':
            sys.exit('target is %s, not Z280' % who)
        self.abort()
        self.until_prompt(30.0)
        r = self.cmd('V')
        if 'Verbose OFF' in r:
            self.cmd('V')
        self.regs = {}

    def close(self):
        os.close(self.fd)

    def abort(self):
        bc.abort()

    def until_prompt(self, cap=20.0):
        buf = b''
        t0 = time.time()
        while time.time() - t0 < cap:
            buf += bc._drain(self.fd, 1.0, 0.12)
            if bc.at_prompt(buf):
                return t(buf), True
        return t(buf), False

    def cmd(self, s, cap=20.0):
        os.write(self.fd, s.encode())
        r, ok = self.until_prompt(cap)
        if not ok:
            raise RuntimeError('no prompt after %r: %r' % (s, r[-200:]))
        return r

    def write(self, addr, data):
        """M: up to 16 bytes at |addr|."""
        assert 1 <= len(data) <= 16
        self.cmd('M%X %s\r' % (addr, ' '.join('%02X' % b for b in data)))

    def dump(self, addr, n=16):
        """{addr: byte} for the 16-byte lines the dump printed."""
        r = self.cmd('d%X %X\r' % (addr, n))
        out = {}
        for line in r.splitlines():
            m = re.match(r'^([0-9A-F]{6}): (.{47})', line)
            if m:
                base = int(m.group(1), 16)
                for i in range(16):
                    cell = m.group(2)[i * 3:i * 3 + 2]
                    if cell.strip():
                        out[base + i] = int(cell, 16)
        return out

    def set_reg(self, name, value):
        r = self.cmd('=%s %X\r' % (name, value))
        if '?Reg' in r:
            raise RuntimeError('register %s rejected' % name)
        self.parse_regs(r)

    def parse_regs(self, txt):
        """The latest register dump in |txt| -> self.regs (only if found)."""
        got = {}
        for line in txt.splitlines():
            for name, val in re.findall(r'\b(PC|SP|USP|BC|DE|HL|IX|IY|MSR|IOP|A|I|R)=([0-9A-F]+)', line):
                if name in ('BC', 'DE', 'HL', 'A') and '(' in line and line.index('(') < line.index(name + '='):
                    continue        # the alternate set, in parentheses
                got[name] = int(val, 16)
            m = re.search(r'F=([SZ1H1VNC_]{8}) ', line + ' ')
            if m and '(' not in line[:line.index('F=')]:
                got['F'] = sum(0x80 >> i for i, ch in enumerate(m.group(1)) if ch != '_')
        if got:
            self.regs.update(got)
        return got

    def reset(self):
        r = self.cmd('R', 30.0)
        self.parse_regs(r)
        self.set_base(force=True)
        # IVTP after a reset is unknown: set it, like any pattern.
        self.write(ORG, [0x0E, 0x06, 0x21, IVTP & 0xFF, IVTP >> 8, 0xED, 0x6E] + [0xFF] * 5)
        self.go()
        self.set_base()

    def set_base(self, force=False, over=None):
        want = dict(BASE_MAP)
        if over:
            want.update(over)
        for name, value in BASE:
            value = want[name]
            if force or self.regs.get(name) != value:
                self.set_reg(name, value)

    def go(self, cap=20.0):
        os.write(self.fd, b'G')
        r, ok = self.until_prompt(cap)
        if ok:
            self.parse_regs(r)
        return r, ok

    def recover(self):
        self.abort()
        time.sleep(0.5)
        who = bc.recover(self.fd)
        if who is None:
            raise RuntimeError('board will not return to its prompt')
        r = self.cmd('V')
        if 'Verbose OFF' in r:
            self.cmd('V')


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


# ------------------------------------------------------------ commands
def cmd_fill(args):
    b = Board()
    prog = lambda seed, first, count: seed + [0x11, first & 0xFF, first >> 8,
                                              0x01, count & 0xFF, count >> 8,
                                              0xED, 0xB0, 0xFF]   # LD DE; LD BC; LDIR; RST 38H
    b.cmd('R', 30.0)
    b.set_reg('SP', 0x2000)   # the exit pushes at 1FFEH, refilled below
    # FF everywhere above the fill program, by the CPU itself.
    b.write(0, prog([0x21, 0x10, 0x00, 0x36, 0xFF], 0x11, 0xFFEF))  # LD HL,0010; LD (HL),FF
    b.set_reg('PC', 0)
    r, ok = b.go(120.0)
    if not ok:
        sys.exit('fill run did not return')
    # Then the two seeded areas, replicated by LDIR from a written seed.
    for lo, hi, seed in ((STACK[0], STACK[1], [0x20, 0x00]),
                         (IVT[0], IVT[1], [0x00, 0x00, 0x40, 0x00])):
        b.write(lo, seed)
        b.write(0, prog([0x21, lo & 0xFF, lo >> 8], lo + len(seed),
                        hi - lo - len(seed)))
        b.set_reg('PC', 0)
        r, ok = b.go(60.0)
        if not ok:
            sys.exit('seed run did not return')
    b.write(0, [0xFF] * 16)
    b.write(0x1FF0, [0xFF] * 16)
    bad = []
    for addr in (0, 0x100, 0x30, 0xC00, 0xFF0, 0x1000, 0x13F0, 0x1FF0, 0x2000, 0x8000, 0xFFF0):
        got = b.dump(addr)
        want = {addr + i: baseline(addr + i) for i in range(16)}
        if got != want:
            bad.append('%04X: %s' % (addr, ' '.join('%02X' % got.get(addr + i, 0x100) for i in range(16))))
    b.close()
    if bad:
        sys.exit('fill check failed:\n' + '\n'.join(bad))
    print('filled: FF, stack %04X-%04X = 20 00, IVT %04X-%04X = {0000,0040}'
          % (STACK[0], STACK[1] - 1, IVT[0], IVT[1] - 1))


def load_done():
    done = {}
    if os.path.exists(OUT):
        for line in zstd.open(OUT, 'rt'):
            try:
                rec = json.loads(line)
            except ValueError:
                continue
            done[rec['key']] = rec
    return done


TIMES = {}


def timed(name, f, *a, **k):
    t0 = time.time()
    try:
        return f(*a, **k)
    finally:
        TIMES[name] = TIMES.get(name, 0.0) + time.time() - t0


def run_one(b, p, variant, over, code, seed, log):
    """One run: load, run, cut, restore. Returns the record."""
    data = list(bytes.fromhex(code)) + [0xFF] * PAD
    timed('write', b.write, ORG, data[:16])
    if seed:
        timed('write', b.write, seed[0], seed[1])
    timed('base', b.set_base, over=over)
    regs = {n: b.regs.get(n) for n, _ in BASE}
    r, ok = timed('go', b.go, 15.0)
    rec = dict(key=key(p, variant), index=p['index'], bytes=code, len=p['len'],
               page=p['page'], opc=p['opc'], mnemo=p['mnemo'],
               operands=p['operands'], variant=variant, regs=regs)
    if not ok:
        b.recover()
        rec.update(end='timeout', cycles=[], raw=r[-1500:])
        b.reset()
        return rec
    cyc = cycles(r)
    if any(c[3] < 0 for c in cyc):
        raise RuntimeError('no S= in the dump: flash a build with -D Z280_PROFILE')
    trace, pushed, end = cut(cyc)
    rec.update(end=end, cycles=trace, exit_pushed=pushed)
    anomaly = []
    if not trace or trace[0][0] != 'R' or trace[0][1] != ORG:
        anomaly.append('leading')
    if end == 'halt' and p['mnemo'] != 'HALT':
        anomaly.append('nohalt')
    # Prefetch past the pattern: overall, and before its last data cycle.
    end_addr = ORG + p['len']
    is_fetch = lambda c: c[0] == 'R' and ORG <= c[1] < end_addr + 16
    last_data = max((i for i, c in enumerate(trace) if not is_fetch(c)), default=-1)
    ahead = lambda cs: max((c[1] for c in cs if is_fetch(c) and c[1] >= end_addr), default=end_addr - 1) - (end_addr - 1)
    rec['ahead_at_exit'] = ahead(trace)
    rec['ahead_before_data'] = ahead(trace[:last_data + 1])
    rec['anomaly'] = ','.join(anomaly)
    # Put back what the run wrote.
    dirty = sorted({c[1] & ~1 for c in cyc if c[0] == 'W'} |
                   ({seed[0] & ~1, (seed[0] + 1) & ~1} if seed else set()))
    for addr in dirty:
        if ORG <= addr < ORG + 16:
            continue                      # rewritten by the next pattern
        timed('restore', b.write, addr, [baseline(addr), baseline(addr + 1)])
    if p['mnemo'] in RESET_AFTER or end == 'halt':
        timed('reset', b.reset)
    else:
        timed('base', b.set_base)
    log(json.dumps(rec) + '\n')
    return rec


def cmd_run(args):
    os.makedirs(OUT_DIR, exist_ok=True)
    pats = patterns()
    done = load_done()
    todo = []
    for p in pats:
        if p['index'] < args.start:
            continue
        if args.only and p['mnemo'] != args.only:
            continue
        for variant, over, code, seed in variants(p):
            k = key(p, variant)
            if args.redo:
                if k not in args.redo:
                    continue
            elif k in done:
                continue
            todo.append((p, variant, over, code, seed))
        if args.repeat and p['index'] % 10 == 0 and p['mnemo'] not in BLOCK_MEM | BLOCK_IO:
            for i in range(1, args.repeat + 1):
                variant = 'rep%d' % i
                if key(p, variant) not in done or key(p, variant) in args.redo:
                    todo.append((p, variant, {}, p['bytes'], None))
    # The disruptive ones last.
    todo.sort(key=lambda x: (x[0]['mnemo'] in RESET_AFTER, x[0]['index']))
    if args.limit:
        todo = todo[:args.limit]
    print('%d runs to do' % len(todo))
    if not todo:
        return
    b = Board()
    b.reset()
    t0 = time.time()

    def log(line):
        # one frame per record: a killed run loses nothing
        with zstd.open(OUT, 'ab') as f:
            f.write(line.encode())

    for n, (p, variant, over, code, seed) in enumerate(todo, 1):
        try:
            rec = run_one(b, p, variant, over, code, seed, log)
        except RuntimeError as e:
            print('%s: %s -- stopped; rerun to resume' % (key(p, variant), e))
            break
        note = rec['end']
        if rec.get('anomaly'):
            note += ' ' + rec['anomaly']
        print('%4d/%d %-12s %-6s %-22s %3d cycles  %s  %.1fs' % (
            n, len(todo), rec['key'], p['mnemo'], p['operands'][:22],
            len(rec['cycles']), note, time.time() - t0))
        sys.stdout.flush()
    b.close()
    compact()
    print('time per step: ' + ' '.join('%s=%.1fs' % kv for kv in sorted(TIMES.items())))


def compact():
    """The recording as one frame, sorted: what gets committed."""
    recs = sorted(load_done().values(), key=lambda r: (r['index'], r['variant']))
    with zstd.open(OUT, 'wt', level=19) as f:
        for r in recs:
            f.write(json.dumps(r) + '\n')


def cmd_status(args):
    pats = patterns()
    done = load_done()
    want = [key(p, v) for p in pats for v, _, _, _ in variants(p)]
    missing = [k for k in want if k not in done]
    ends = {}
    for rec in done.values():
        ends[rec['end'] + (' ' + rec['anomaly'] if rec.get('anomaly') else '')] = \
            ends.get(rec['end'] + (' ' + rec['anomaly'] if rec.get('anomaly') else ''), 0) + 1
    print('%d patterns, %d runs wanted, %d recorded, %d missing'
          % (len(pats), len(want), len(done), len(missing)))
    for k, v in sorted(ends.items()):
        print('  %-24s %d' % (k, v))
    if missing:
        print('missing:', ' '.join(missing[:20]), '...' if len(missing) > 20 else '')


# ------------------------------------------------------------ check
SAMPLES = os.path.join(PROJ, 'samples', 'z280')
# How each sample is driven: what to type, or None to halt it after a while.
CHECKS = [('arith', None), ('mandelbrot', None), ('echo', b'ok'),
          ('echoir', b'irq'), ('echoitr', b'az')]


def listing(name):
    """address -> hex bytes of every instruction in the sample's listing."""
    starts = {}
    for line in open(os.path.join(SAMPLES, name + '.lst')):
        m = re.match(r'^(?:\(\d\))?\s*([0-9A-F]+) : ((?:[0-9A-F]{2} )+)\s*(\S+)', line)
        if m and m.group(3).lower() not in ('org', 'db', 'dw', 'ds', 'equ', 'include', 'end'):
            starts[int(m.group(1), 16)] = m.group(2).replace(' ', '')
    return starts


def cmd_check(args):
    b = Board()             # verbose: the raw cycles are kept for the host
    failed = 0
    for name, feed in CHECKS:
        b.cmd('R')
        bc.upload_file(b.fd, os.path.join(SAMPLES, name + '.hex'))
        os.write(b.fd, b'G')
        if name == 'arith':
            r, ok = b.until_prompt(60.0)     # runs to its own exit
        else:
            time.sleep(1.5)
            bc._drain(b.fd, 0.5, 0.2)
            if feed:
                for ch in feed:
                    os.write(b.fd, bytes([ch]))
                    time.sleep(0.2)
                bc._drain(b.fd, 0.5, 0.2)
                os.write(b.fd, b'\x00')  # the samples exit on NUL
                r, ok = b.until_prompt(20.0)
            else:
                b.abort()
                r, ok = b.until_prompt(40.0)
        with open(os.path.join(OUT_DIR, 'check-%s.txt' % name), 'w') as f:
            f.write(r)          # for the host harness, when a line is wrong
        starts = listing(name)
        lines = [l for l in r.splitlines() if re.match(r'^[0-9A-F]{6}: ', l)]
        wrong = []
        for l in lines[:-1]:            # the last one is the register dump's
            addr = int(l[:6], 16)
            code = ''.join(re.findall(r'^[0-9A-F]{6}: ((?:[0-9A-F]{2} )+)', l)).replace(' ', '')
            if starts.get(addr) != code:
                wrong.append(l[:40].strip())
        raw = sum(1 for l in r.splitlines() if CYC.match(l) and not l.lstrip().startswith(tuple('0123456789')) or CYC.match(l))
        # A cut instruction at the ring's start may decode as a few
        # lines: allow that at the top, nowhere else.
        last_wrong = max((k for k, l in enumerate(lines[:-1]) if l[:40].strip() in wrong), default=-1)
        ok = len(wrong) <= 5 and last_wrong < 6 and len(lines) > 10
        failed += not ok
        print('%-11s %-4s %3d instructions, %d wrong, %3d raw cycles%s' % (
            name, 'ok' if ok else 'BAD', len(lines) - 1, len(wrong), raw,
            ('  ' + ' | '.join(wrong[:3])) if wrong else ''))
    b.close()
    sys.exit(1 if failed else 0)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('cmd', choices=['fill', 'run', 'status', 'check'])
    ap.add_argument('--only')
    ap.add_argument('--redo', action='append', default=[])
    ap.add_argument('--from', dest='start', type=int, default=0)
    ap.add_argument('--limit', type=int, default=0)
    ap.add_argument('--repeat', type=int, default=0)
    args = ap.parse_args()
    {'fill': cmd_fill, 'run': cmd_run, 'status': cmd_status, 'check': cmd_check}[args.cmd](args)


if __name__ == '__main__':
    main()
